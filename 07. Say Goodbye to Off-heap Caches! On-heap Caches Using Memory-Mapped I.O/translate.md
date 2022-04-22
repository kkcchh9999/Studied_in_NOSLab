# 
## Say Goodbye to Off-heap Caches! On-heap Caches Using Memory-Mapped I/O

## Abstract
많은 분석적 연산이 수렴 조건이 충족될 때 까지 실행되는 반복 처리 단계에 의해 지배된다.
느린 DRAM 용량 증가와 데이터가 기하급수적으로 증가할 때 워크로드를 가속하기 위해 Spark는 중간 결과의 off-memory caching을 사용한다. 
하지만, off-heap caching은 데이터셋의 크기가 증가할때 특히 상당한 오버헤드를 가하는 데이터의 직렬화/역직렬화(SerDes)를 필요로 한다.  

이 논문은 Spark 데이터 캐시의 확장으로, memory-mapped I/O를 통해 모든 cache 데이터를 on-heap이며 off-memory 상태로 저장함으로 serdes의 필요성을 피하는 TeraCache를 제안한다.
이를 이루기 위해, TeraCache는 memmory-mapped 된 고속 저장 장치에 상주하고 캐시된 데이터만 명시적으로 사용되는 managed heap을 통해 오리지널 JVM heap을 확장한다.
예비 결과는 TeraCache 프로토타입이 최신의 serdes 접근에 비해 중간 결과 약 37% 가량 Machine Learning 워크로드의 속도를 향상시킬 수 있는 것으로 나타났다. 

## 1. Introduction 
분석 어플리케이션들은 많은 양의 데이터를 처리하기 위해 종종 ML(머신 러닝) 알고리즘을 사용한다.
이런 어플리케이션들은 수렴 조건을 충족할 때 까지 한번 혹은 그 이상의 연산 과정을 반복한다.
이런 워크로드의 속도를 가속하기 위해 Spark는 복잡한 연산 파이프라인의 중간 결과를 caching하고 각 단계에서 재사용한다.
중간 결과는 RDDs(Reslient Distributed Datasets)로 LRU cache에 저장된다.  

그림 1은 세가지의 잘 알려진 ML 워크로드에서의 RDD caching의 성능을 보여준다 : Linear Regression(LR, 선형 회귀), Logistic Regresson(LgR, 로지스틱 회귀), Support-Vector Machines(SVM, 서포트 벡터 머신)
모든 워크로드는 64GB의 데이터셋과 32GB DRAM과 30개의 CPU 코어를 사용하는 단일 Spark Excutor에서 실행되었다(우리는 3회 실행한 평균값을 기록했다. 편차가 최소화되어 생략되었다).
메모리와 디스크 모두를 사용하는 환경에서(hybrid) RDDs Caching은 중간 결과를 모두 다시 계산하는 것에 비해 약 90%의 성능을 향상시켰다.     

DRAM에만 caching하는 것은 장기적인 해결책이 아닌데, 이는 DRAM 확장성이 한계에 도달하는 동안 생성되고 처리되는 데이터의 양이 높은 비율로 증가하기 때문이다. Cached된 데이터는 점진적으로 DRAM의 물리적 크기를 초과하고, 각 단계에서 중간 결과를 재연산하게 만들기 쉽다. 
그러므로 현재 관행은 Spark의 중간 RDDs를 저장하기 위한 cache 사이즈를 증가시키기 위해서 빠른 저장 장치를 사용하는 것이다.
SSD나 NVM 뿐만 아니라 NVMe 블록 장치같은 NAND flash storages는 DRAM보다 더 높은 용량과 밀도를 가진다.
DRAM은 각 DIMM 슬롯당 GB 규모인데 반해 SSD와 NVMe 장치들은 더 적은 비용으로 각 PCIe slot 당 terabytes의 규모를 가진다.
더욱이, NVMe bloc device는 NVM과 비교했을 때 적은 비용으로 더 높은 용량을 제공하며, 몇 마이크로초 정도의 접근 지연을 가지며 읽기 및 쓰기 모두 수백만 IOPS를 수행한다.  

실행 중에, Spark는 최초에 RDDs를 heap(DRAM)에 위치시킨다.
만일 하나의 RDD 블록이 메모리에 맞지 않으면 이는 off-heap(disk) 캐시로 직렬화되고 메모리는 다음 가비지 컬렉션에 회수된다.
유사하게, 만일 충분한 메모리가 존재하지 않으면 LRU 캐시는 오래된 항목을 off-heap 캐시로 퇴거시킨다.
Spark가 off-heap에 저장된 RDD를 참조할 때, 디스크에서 직렬화된 블록을 메모리로 역직렬화한다.
RDDs는 변경할 수 없기 때문에 모든 블록은 최대 한번 직렬화된다.
하지만, 각 블록마다 역직렬화는 반복적인 단계에서 여러 번 일어날 수 있다.  

오늘날, 뛰어난 재계산 전략에도 불구하고, off-heap 장치에 caching하는 것은 높은 serdes 오버헤드를 발생시킨다.
Zhang에 따르면 serdes가 디스크 I/O보다 오버헤드를 지배한다.
그림 1 (b)는 LR, LgR, SVM에서 RDDs를 off-heap에만 저장할 때 RDD cache의 성능을 보여준다.
off-heap caching과 serds의 오버헤드의 큰 영향을 주의해라.
serdes는 disk를 사용할 때의 전체 실행 시간의 평균 27%를 차지한다.
역직렬화 연산이 전체 serdes 연산의 80~90%를 차지하는데, 이는 워크로드가 변경할 수 없는 cached된 RDDs를 매 반복마다 장치에서 검색하기 때문이다.
Serdes 오버헤드는 저장 장치의 속도가 빨라지고 CPU와 메모리 성능의 격차가 좁혀질수록 악화되었다. 
전체 실행 시간 또한 주로 병렬 처리가 감소하고 디스크 처리량이 30 CPU 코어를 유지할 수 없기 때문에 CPU가 idle한 시간으로 인해 디스크에서만 caching할 때 증가되었다.   

on-heap cache(DRAM)를 함께 쓰는 방법은 serdes 비용을 줄여주지만, 여전히 GC 오버헤드가 존재한다. 
그림 1(b)는 현재 흔히 사용하는 방식인 RDDs를 하이브리드 cache에 저장할 때의 성능을 보여준다. 
비교적 큰 on-heap cache를 사용하는것은(Spark는 60%의 힙 공간을 캐시로 비축한다), 일부 RDDs를 메모리에 저장함으로 스토리지에 저장하는 것 보다 평균적으로 20% 정도의 상당한 serdes 오버헤드 감소를 보여준다.
하지만, 이런 큰 on-heap cache는 스토리지에만 저장할 때 보다 SVM에서 13배, LgR에서 36배 정도의 GC 시간을 증가시킨다.
Cached된 RDDs는 최초에 힙에 위치하고, 높은 비율의 수명이 긴 객체를 수명이 짧은 객체로 만든다.
따라서 GC는 JVM 힙에 살아있는 객체를 마킹하는데 시간을 더 소모하고, 힙의 큰 비율은 RDDs로 가득 차 있기 때문에 적은 비율의 메모리 회수를 끝낸다.
본질적으로, Spark는 실행과 cache 메모리를 위해 DRAM-only JVM 힙을 사용한다.
이것은 예측할 수 없는 성능 혹은 실패를 초래할 수 있는데, 큰 데이터를 캐싱하는 것은 실행 중에 추가 GC 부담을 초래하기 때문이다.   

이 연구에서 우리는 RDD caching이 메모리에서 고속 스토리지 장치에 매핑되며 GC되지 않는 힙의 일부분의 크고 관리되는 on-heap 캐시에서 수행되어야 한다고 주장한다.
TeraCache는 JVM 힙을 실행 부분과 cache 부분으로 분할하여 실행 heap은 DRAM에 위치시키고, cache부분은 DRAM-mapped block device에 위치시킨다. 
이는 본질적으로 serdes와 모든 관련된 오버헤드를 제거하고 cached된 데이터의 GC를 예방하며, 장치 기술 동향 및 서버 전력 제한과 일치한다.   

다음으로, 우리는 Spark에서 DRAM이 실행과 cache 사이에서 어떻게 나누어야 하는지를 관찰한다.
명백하게, 힙의 실행 부분은 작업 수행 중에 GC에 대한 부담을 유발하지 않기 위해 충분한 DRAM을 사용해야 한다.
반대로, cache 힙으로 DRAM을 많이 사용할수록, cached된 데이터에 더 빠른 접근이 가능하다. 
우리는 JVM이 실행과 caching을 모니터링하여 동적으로 DRAM의 사용을 조정하는 디자인을 제안한다.  

우리의 접근 방식 TeraCache는 on-heap RDD 캐싱과 memory-mapped 고속 스토리지 장치의 공동 설계 접근방식이다.
TeraCache는 Spark cache, JVM 메모리 관리, GC, MMIO(Memory Mapped I/O)에 걸쳐 있다. 
TeraCahce의 주요 이점은 다음과 같다:
 - 큰 memory-mapped 힙에 caching함으로 JVM이 cached된 데이터를 직접 load/store 할 수 있기 때문에 serdes의 필요성을 제거한다.  
 - cached된 RDDs를 유지함에도 JVM 힙을 분리하여 사용하기 때문에 GC의 빈도와 길이를 줄인다.  
 - MMIO 및 실행에 사용되는 DRAM을 동적으로 조정하여 실행 속도와 I/O 오버헤드의 균형을 조절한다.  

우리는 Spark의 caching을 목표로 하는 TeraCache의 초기 프로토타입을 구현했고, 반복적인 ML 워크로드에서 효용성을 확인하기 위해 DRAM의 동적 크기 재구성을 조사했다.
우리의 검증은 현재 최신의 하이브리드 접근 방법에 비해 37%의 성능 향상을 보여준다.   

## 2. TeraCache: Caching Over a Device Heap
Spark는 하나의 드라이버와 여러개의 executor 프로세스로 구성되어있다.
드라이버는 executor이 드라이버의 작업을 수행하는 동안 모든 명령을 생성하는 Spark 유저들로부터 실행되는 메인 프로세스이다. 
그림 2는 Spark가 excutor 메모리를 어떻게 두가지 논리적 파트로 나누는지를 나타낸다: (1) 실행 메모리, 연산 중 임시 데이터를 저장하기 위함, (2) 저장 메모리, 중간 RDDs를 caching하기 위함(LRU 캐시, Least Recently Used).
각 executor는 JVM 인스턴스에서 실행되고 DRAM에 위치하는 JVM 힙의 메모리를 할당받는다.
하나의 RDD가 storage 메모리에 맞지 않을 때 직렬화되어 디스크로 보내진다.   

우리의 연구는 executor 메모리의 연산, caching 두 가지를 사용한다.
우리는 물리적으로 JVM 힙을 이 두 가지 규칙에 따라 분할했다.
그리고, 우리는 JVM 힙의 각 부분을 메모리 계층 내부의 자원을 특정하기위해 그림 2(b)와 같이 매핑했다: (1) JVM Heap(H1)는 독점적으로 DRAM(DR1)에 할당되고 generator에 의해 분할될 수 있다(예: New, Old) (2) cached된 RDDs 모두를 저장하고, 크기는 장치의 용량에 따라 제한되는 커스텀 managed heap(TeraCache Heap)  

mmap에서 사용되는 페이지를 위한 메모리 매핑은 DRAM(DR2)의 남아있는 부분에 위치한다. DR1과 DR2는 실행 중에 적응 정책에 따라 TeraCache를 통해 동적으로 조정된다.
이를 위해 TeraCache는 아래에서 묘사한 것과 같이 Spark Block Manager, JVM의 가비지 컬렉터의 여러 확장을 요구한다.  

### 2.1 Design Challenges
**NVMe에서 mmap을 이용한 TeraCache 힙 할당은 현실적이다:** 우리는 JVM을 추가적인 힙을 할당하도록 수정했고, 메모리 매핑을 고속 스토리지에 memory-mapped 되도록 수정했다(예를 들어 NAND-Flash SSD or NVMe에 Linux mmap 를 이용하여).
고속 SSD나 NVMe 장치는 HDD와 반대로 생산되는 접근 패턴과 장치의 특성 때문에 mmio를 받을 수 있다.
그림 3(왼쪽)은 LgR을 HDD와 NVMe에서 serdes와 mmap을 수행했을 때의 성능을 보여준다.
두 가지 경우 모두 우리는 18GB의 데이터셋을 사용했고 8GB DRAM과 30개의 코어에서 하나의 Spark executor을 사용했다.
실제 cached 되어야 하는 훈련 set은 DRAM 캐시 크기의 10배이다.
평균 요청 크기를 보여주는 그림 3(오른쪽)에 나타나듯, mmap은 작고(page 크기 까지 (4KB)) serdes에 비해 비교적 랜덤한 I/O를 제공한다.
HDD는 이 접근 패턴에서 잘 작동하지 않는다. 
3배 큰 request 크기를 가진 serdes는 더 큰 serdes CPU 오버헤드를 가짐에도 불구하고 mmap보다 3배 더 낫다.
하지만 NVMe는 작은 request 크기에 대해 접근 패턴과 관계 없이 높은 처치량과 낮은 지연을 달성하여 serdes에 비해 mmap 36% 나은 성능을 나타냈다.
우리는 FastMap과 같은 최적화된 mmio를 사용함으로 성능을 더 향상시킬수 있을 것이라 기대한다.  

serdes를 피하기 위해 JVM의 힙을 키우는 다른 방법은 OS 가상 메모리 시스템을 사용하여 NVMe를 스왑 공간으로 사용하는 것이다.
비록 이것은 NVMe에 매우 큰 JVM 힙을 저장할 수 있더라도, 이는 RDD 캐시 오브젝트 목표로 단독으로 사용될 수 없고, 또한 GC를 피할 수 없다.
TeraCache가 실행과 caching을 위한 두 분리된 힙들을 사용하기 때문에, 명시적으로 cache의 GC를 피하고 저장장치에 cache 하기 위해 mmap은 더 적합한 메커니즘이다.
그림 4(a)는 힙의 실행 및 캐싱 부분에 대해 별도의 힙을 유지 관리하고, 캐싱에 NVMe를 엄격하게 사용하고, 캐시에서 GC를 방지하는 것이 모두 성능에 중요함을 보여준다.
TeraCache는 간단하게 큰 단위로 GC를 하면서 단독 JVM을 쓸 때 보다 최대 2배 향상을 보였다.  

**TeraCache 힙 관리는 비용이 큰 GC를 피한다:** 
JVM은 실행-저장 메모리 분리를 인식하지 못하기 때문에, 모든 객체는 JVM 힙 공간에 할당된다(그림 2(a)의 중간 레이어).
이는 다음과 같은 이유로 GC 시간을 증가시킨다: (1) cached 된 RDDs가 오랜 수명을 가진 객체의 모음이고, Spark 응용 프로그램의 명시적 지속 및 비지속 작업으로 관리된다.
그러므로, 그들은 GC에 드물게 수집되고, 개체를 탐색하는데 상당한 시간이 걸린다; 그리고, (2) 실행 메모리에 충분한 회수를 위해 더 많은 GC 사이클을 요구하는데, 이는 수명이 긴 RDD로 인해 각 GC 사이클이 적은 양의 메모리를 회수하기 때문이다. 
cached 된 객체의 생명 주기는 Spark 캐시에 얼마나 오래 유지되는지에 따라 명백하게 정의되어, 우리는 TeraCache에서 GC를 피할 수 있다.
따라서 우리의 디자인은 TeraCache를 관리하기 위해 커스텀 allocator를 사용하고, 다음과 같이 객체 회수를 줄인다.  

우리는 가비자 컬렉터를 증강하고 H1과 TeraCache의 차이를 인지시켰다. 
H1은 기본적인 JVM 힙으로 처리되고 일반적인 GC 알고리즘을 사용하여 수집된다. 
TeraCache는 커스텀한 지역 기반  allocator를 사용하고, 자바 메모리 모델을 준수한다.
특히, 우리는 TeraCache를 RDDs에 상응하는 지역으로 구성했다.
따라서, TeraCache는 Broom과 비슷하게 동일한 지역에 동일한 생명 주기를 갖는 객체를 배치단위로 한번에 free 할 수 있다. 
메모리 안전성을 유지하기 위해, 우리는 TeraCahce에서 H1을 참조할 수 없게 했다.
우리는 각 RDD 객체와 이 객체에서 도달할 수 있는 모든 객체를 TeraCache로 이주하여 이를 해결했다.
그러면, GC는 H1의 유효한 객체를 찾기 위해 TeraCache를 횡단할 필요가 없다.
RDDs는 항상 다른 객체를 오염시키지 않고 변하지 않고 안전하게 TeraCache로 이동할 수 있다.  

**RDDs를 TeraCache 에 Caching하는것:** 
RDD caching은 명시적으로 개발자에 의해 Spark RDD API를 통하여 관리됨으로, 우리는 두 가지 Java 에노테이션을 소개한다, @cache와 @uncache,는 Spark Block Manager상응하는 코드 포인트에 에노테이트 하기 위해. 