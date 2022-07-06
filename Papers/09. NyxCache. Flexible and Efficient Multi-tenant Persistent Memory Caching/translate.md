# NyxCache: 유연하고 효과적인 멀티테넌시 영구 메모리 caching
## 09. NyxCache: Flexible and Efficient Multi-tenant Persistent Memory Caching

## Abstract
우리는 NyxCache(Nyx)를 제안한다. 이는 multi-tenant 영구 메모리 caching을 위한 접근 제한 프레임워크이며, 경량화된 접근 규제, cache별 리소스 사용량 추정 및 cache 간 간섭 분석을 지원한다.
이러한 메커니즘 그리고 기존의 admission control 및 capacity allocation logic을 사용하여, 우리는 resource-limiting, QoS-awareness, fair slowdown, proportional sharing과 같은 핵심 sharing policies를 빌드했다: Nyx resource-limintng은 bandwidth limiting method에 비해 5배 더 나은 성능 고립을 제공하며 각 cache의 PM 사용을 정확하게 제한한다. 
Nys QoS는 간섭받지 않는 best-effort cache들에게 더 높은(기존의 DRAM 기반의 접근법에 비해 6배 이상) 처리량을 제공하면서 지연성이 중요한 cache들에게 QoS 보장을 제공할 수 있다.
마지막으로, 우리는 Nyx가 향상된 best-effort 트래픽으로 인해 중요한 cache가 느려지지 않음을 보장하고, writing spikes를 고립하여 현실적인 워크로드에서 유용함을 보여준다.
 
## 1. Introduction
메모리 기반 키밸류 cache는 많은 어플리케이션과 시스템의 핵심 컴포넌트이다. 
활용성을 향상시키고 관리를 쉽게 하기 위해 다중 cache 인스턴스는 종종 multi-tenant 서버에 통합되었다. 
예를 들어, Facebook, Twitter는 각각 수천개의 cache 인스턴스를 호스팅하는 수백개의 전용 cache 서버를 유지한다.
하지만, multi-tenant 서버는 각 client cache가 성능 목표를 충족하는지 확인해야 하는 문제점이 있다; 현재 메모리 용량 및 대역폭 제한, 서비스 품질 보장, 그리고 리소스 비례 할당과 같이 다양한 생산 및 연구 인메모리 다중 테넌트 cache가 서로 다른 공유 정책을 제공한다.  

Intel의 Optane DC PMM과 같은 영구 메모리(PM)는 이러한 cache에 대해 매력적인 구성 요소로 떠오르고 있는데, 이는 PM의 큰 용량, 낮은 byte 당 가격, 그리고 DRAM에 견줄 만 한 성능 때문이다.
하지만, PM의 성능은 DRAM과 Flash에 비해 여러 방면에서 다른데, 이는 현재의 multi-tenant cache의 효과를 줄인다.
특히, DRAM과 달리, Optane DC PMM은 read와 write의 성능이 매우 비대칭적이고(하나의 DC PMM의 경우, 최대 read 성능은 6.6GB/s 인 반면, write는 2.3GB/s 이다.), 읽기와 쓰기간의 심각한 간섭이 존재하며(1GB/s를 write 하는 것은 동일한 처리량과 P99 지연 속도 저하가 동시 실행 읽기 워크로드를 8GB/s 로 읽는 것 처럼 유발할 수 있다.), 특히 256B의 배수에 대한 효율적인 접근이 가능하다.  

불행히도, 기존의 multi-tenant DRAM과 스토리지 caching 기술은 PM에 유용하게 변경되지 않았다.
몇몇 접근법은 전체 client의 용량 할당에만 집중되어 있다; 용량 할당은 필수적이지만, PM 공유에는 충분하지 않았는데, 이는 PM에 대한 요청의 비율이 제한되어야 하기 때문이다.
Host-level의 request 제한은 block-layer I/O 스케줄링을 사용하는 Flash 장치에 대해 광범위하게 연구되었는데, 하지만 이러한 소프트웨어의 오버헤드는 100ns PM 액세스를 고려할 때 너무 많다.
Device-level request 스케줄링은 PM에 부족한 특별한 하드웨어를 가정한다.
마침내, coarse-grain request throttling은 DRAM 대역폭 할당의 대다수를 뒷받침한다;하지만, 이러한 접근은 하드웨어 카운터와 성능 특성 모두가 PM을 유지하지 않는다고 가정한다(예를 들어, 대역폭은 활성화의 정확한 평가이다). 

이 논문에서, 우리는 multi-tenant key-value caches를 위한 독립적이며 가볍고 유연한 PM 접근 제한 프레임워크인 NyxCache(Nyx)를 소개한다. 이는 별도의 하드웨어 지원 없이 오늘날의 PM을 위해 최적화되어 있다.
PM 서버와 공유 정책(예를 들어 QoS)이 주어지면, cache instance들은 기존의 load admission과 capacity allocation 기술을 사용하여 admit 되고 공간을 할당받는다.
실행 시에, Nyx는 cache의 정보를(예를 들어 PM 자원 활성화) 모니터링하고, 각 cache가 PM에 접근하는 비율을 제한하며, 따라서 공유 정책의 성능 목표를 강제한다.
Nyx는 memcache 인터페이스를 준수한다면 어떠한 in-memory key-value 저장 장치에서 동작한다; 현재 구현된 Twitter의 Pelikan 버전은 PM에 최적화되었으며, 이는 싱글 cache 성능을 50% 가량 향상시키고, 쓰기 집약적 워크로드의 경우 3배 이상 향상시킨다.
Nyx의 핵심적인 기여는 PM이 대중적인 공유 정책을 유연하게 시행하는 데 필요한 정보를 추출하도록 설계된 일련의 소프트웨어 메커니즘이다.  

Nyx는 새로운 메커니즘을 효율적으로 제공한다:
- i) PM 접근 제한
- ii) client의 PM 자원 사용량 얻기
- iii) client 내부 인터페이스 분석
또, PM을 위한 특히 유용한 두 가지의 새로운 메커니즘을 제공한다.
먼저, Nyx는 공유 정책에 필요한 각 cache instance로 인해 발생한 전체 PM DIMM 활성화 뿐만 아니라 PM 활성화를 효율적으로 추정한다; PM 활성화를 추정하는 것은 어려운데, 이는 DRAM과 달리 수많은 전송된 bytes들이 정확한 PM 활성화를 의미하지 않기 때문이다. 
두 번째로, Nyx는 다른 cache instance를 가장 많이 방해하는 cache를 확인할 수 있다; PM 기반 시스템에서, 이러한 상호작용은 정의하기 어려운데, 이는 DRAM과 달리 손상된 client가 높은 대역폭의 client보다 낮은 대역폭의 client에 의해 더 많은 영향을 받을 수 있기 때문이다.
이러한 두 메커니즘은 PM에서 높은 성능을 위해 필수적인 CPU cache prefetching을 정확히 설명한다.
이러한 새로운 메커니즘은 Nyx가 쉽고 효율적으로 resource limiting, QoS, fair slowdown, proportional sharing 등의 공유 정책을 지원하도록 한다.  

Nyx에서 제공하는 공유 정책은 강력하다.
Nyx는 정확하게 각 cache의 PM 활성화를 제한할 수 있는데(Google Cloud의 메커니즘과 유사하게) 반면, 대역폭을 측정하는 방법으로는 불가능하다.
Nyx는 best-effort cache에 (6배 이상)높은 처리량을 제공하는 동시에 간섭 없이 latency-critical caches에게 QoS를 보장할 수 있다.
Nyx는 다른 작업들이 느려지지 않게 하면서 idle한 PM utlization을 client에게 재분배 하면서 proportional resource allocation을 제공할 수 있다.
마지막으로, Twitter의 실제 대규모 cache 추적에서 볼 수 있듯, Nyx는 client를 write spikes로 부터 고립시켜 중요한 cache가 증가된 best-effort 트래픽으로 인해 느려지지 않게한다.    

이 논문의 이후는 단락 2에서 기존의 multi-tenant cache와 PM에 대한 한계를 검증하고, 단락 3에서는 Nyx 디자인에 대해 논의, 단락 4에서는 Nyx 메커니즘의 오버헤드와 정책의 효율성을 검증하며, 단락 5에서는 잠재적인 확장을 논의하고, 단락 6에서는 관련 연구, 단락 7은 결론이다.  

## 2 Motivation and Challenges
우리는 많은 in-memory multi-tenant key-value cache로부터 제공되는 sharing policie와 이를 구현하기 위한 메커니즘에 대한 배경 지식을 제공한다.
우리는 왜 DRAM 혹은 block I/O에서 제어 및 정보를 제공하기 위한 이전의 접근이 PM에서 잘 동작하지 않는지 설명한다.  

### 2.1 Sharing Policies for Multi-Tenant Caches
memcached, Redis, Pelikan과 같은 In-memory key-value cache는 많은 실시간 배치 어플리케이션을 위한 웹 인프라에서 필수적인 부분이다.
느린 백엔드 스토리지 혹은 연산 노드들에서 데이터에 접근하기 전에, 어플리케이션은 먼저 in-memory cache를 확인한다.
production의 환경에서, cache 서버는 주로 multi-tenant이다: 많은 cache 인스턴스는 활성화를 향상시키고 관리를 편하게 하기 위해, 또 scaling을 위해 단일 서버에 통합되어 있다.
multi-tenant cache에서, request는 상응하는 cache 인스턴스로 라우팅된다.
예를 들어, Facebook이나 Twitter과 같은 거대한 기업은 수천개의 cache 인스턴스를 호스팅하기 위해 수백개의 거대한 메모리 전용 서버를 유지한다. 
비슷한 기업들은 ElastiCache, Redis, Memcachier과 같은 caching-as-a-service providers를 사용한다.
이 논문에서, 우리는 개인의 multi-tenant cache 서버에 주목한다.    

multi-tenant caching에서 경쟁하는 client에게 성능을 주는 것과 목표를 달성하는 것은 중요하다.
다른 산업 그리고 연구 multi-tenant 시스템은 다른 오브젝트들을 제공한다; 우리는 다음 4가지에 주목한다.
 __Resource Limiting__: clients가 리소스 비용을 지불할 때 일반적인 목표는 각 client가 대역폭, ops/sec, 혹은 리소스의 수와 같은 몇몇 사용량을 초과할 수 없도록 보장하는 것이다.
 예를 들어, Google Cloud memcache는 "GB당 초당 최대 10K의 읽기 또는 5K의 쓰기(독점)" 처럼 결제 티어에 따라 명령을 제한한다.
 여러 자원은 동시에 제한될 수 있다, 예를 들어, Amazon ElastiCache는 메모리와 vCPUs 모두에 요금을 부과한다.  

 multi-tenant cache가 각 client에 자원을 제한하기 위해 두 가지 요구사항이 존재한다. 
 먼저, 시스템은 각 client가 사용하는 자원의 양을 정확하게 판단해야 한다; 우리는 이를 _resource usage estimation_ 이라고 부른다.
 두 번째는, 각 client의 요청이 한계를 초과할 경우, 요청을 제한하거나 reschedule 해야하는데, 우리는 이를 _request regulation_ 이라고 부른다. 
 아래에서(2.2절) 우리는 어떻게 이전의 multi-tenant 캐시가 request regulation과 resource usage estimation을 제공했는지 그리고 왜 이런 이전의 접근이 PM에 적합하지 않는지를 설명한다.  

 __Quality-of-Service__: multi-tenant 시스템은 Twitter와 Microsoft처럼 함께 배치된 다른 클라이언트와 상관 없이 각 클라이언트의 성능 목표를 달성할 수 있다.
 이 목표는 service-level 목표를 달성해야 하는 latency-critical clients에게 유용하다.
 예를 들어, Twitter에서의 production cache는 p999 5ms 이하의 레이턴시를 제공한다. 

 QoS를 제공하는 것은 각 client가 실행 시 목표에 도달했는지 아닌지를 알 필요가 있다.
 시스템이 하나의 보장된 성능에 도달하지 못한 client를 관측하면, 영향을 끼치는 client는 식별되고 제한된다. 
 대부분의 악영향을 미치는 클라이언트를 식별하는 것은 DRAM의 경우, 주로 직관적이며 간단하게 대역폭을 기반으로 하고 있지만, PM은 그렇지 않다.
 PM에서 워크로드를 조율하기 위해 _inference estimation_ 을 포함하는 새로운 기술이 요구된다.   

 추가적인 실행 보조로, QoS를 보장하는것은 접근 제어 및 공간 할당을 요구한다.
 접근 제어는 새로 도착하는 client가 시스템이 충분한 자원을 가지고 있음을 확신하고, 새로운 client가 기존의 client를 방해하지 않도록 수행되어야 한다.
 cache instances에서 공간 할당은 각 client가 목표를 달성하기 위해 지정된 hit ratio를 제공해야 한다. 
 이전 연구는 이러한 문제에 초점이 맞추어 져 있었다.
 예를 들어, Microsoft는 QoS 대역폭을 목표로 공간을 할당했고, Robinhood는 tail latency를 최소화했다. 접근 제어와 공간 할당은 대부분 새로운 문제점과 직교하며, 우리는 여기에 집중하지 않는다.   

 __Fair Slowdown__: 더 협력적인 환경에서의 multi-tenant 시스템은 모든 client가 동등한 양만큼 속도가 저하될 수 있다.
 공식적으로, 이런 접근은 최대 속도저하대 최소 속도저하의 비율을 최소화한다. 
 web cache 설정에서, 어플리케이션 요청이 fan out 될 수 있으며, 이 때 가장 긴 지연성을 가진 cache가 전체 지연 시간을 결정한다; 따라서, 균형잡힌 속도 저하는 전체 request 지연성에서 이점을 갖는다.  

 fair slowdown을 강제하가 위해서는 각 cache의 속도 저하를 알아야 한다.
 시스템은 서버를 공유할 때 각 cache의 현재 성능을 모니터해야하고, 혼자 동작할때의 성능을 알아야 한다.
 _slowdown estimation_ 을 위한 기술이 요구된다.
 게다가, 다른 cache간의 속도 저하를 동등하게 하기 위해 속도 저하가 적은 cache의 경우 더 많은 제한이 필요하고, 속도 저하가 큰 cache의 경후 제한을 줄여야 한다(예를 들어, _request regulation_).  

 __Proportional Resource Allocation__: 마지막으로, multi tenant 시스템은 N개의 client가 각각이 독립 성능의 1/N 이내에 서 실행되도록 보장함으로써 client가 자원을 공유하도록 유도할 수 있다.
 이 보장은 각 client가 다른 비율을 공유함으로 일반화될 수 있다.
 idle 자원은 client들에게 재분배되고, 몇몇 client는 그들이 보장받은 것 보다 더 많은 리소스를 얻을 수 있다.
 예를 들어, FariRide는 proportional cache 할당을 하게 한다.  

 비례하는 할당을 보장하기 위해 multi-tenant cache는 세 가지 요구사항을 만족해야 한다. 
 시스템은 각 client가 할당된 것 이상의 자원을 소모하지 앟ㄴ게 하기 위해서 _request regulation_ 과 _resource usage estimation_ 을 수행해야 한다.
 idle한 자원을 client에게 할당할 때, 시스템은 반드시 다른것들을 방해하지 않는지 검증해야 한다; 따라서, 시스템은 속도 저하를 추적해야 하고(다시 말해, slowdown estimation), client에 영향을 미치기 전에 idle 한 자원 재분배를 중단해야 한다.  

 요약하자면, multi-tenant cache가 위의 정책들을 제공하기 위해, 각 cache 인스턴스의 자원 사용을 제어하고, 자원 및 어플리케이션 성능에 대한 정보를 얻어야 한다.
 표 1은 각 정책이 필요로 하는 정보 및 컨트롤에 대한 요약이다.  

### 2.2 Challenges of PM Cache Sharing
PM은 key-value cache에게 매력적인 요소이다.
PM의 배경지식에 대해 이야기 한 후, 우리는 multi-tenant cahcing에 PM을 사용하기 위한 문제점을 설명한다.  

#### 2.2.1 Persistnent Memory Characteristics
PM은 연구 프로토타입 및 제품으로 현실이 되고 있다. 
예를 들어, Intel Optane DC PMM은 인기있는 사용 가능한 장치이다; 또한, 연구 프로토타입도 존재한다.
이 논문에서, 우리는 Optane DC PMM을 사용한다.
PM 성능은 DRAM과 비슷하지만, 더 많은 용량과 더 낮은 가격을 제공한다.
PM은 NAND Flash에 비해 상당히 빠르며 byte단위로 주소를 지정할 수 있다. 
PM은 메모리 버스에 직접 연결되어 있으며, App Direct Mode로 구성되어 있을 때, load와 store로 접근될 수 있다. 
PM 접근을 위한 다른 CPU caching 옵션도 존재한다: CPU caching과 prefetching을 통한 load 와 store; prefetching disabled된 load 와 store(PM과 DRAM 모두); CPU cache 전체를 우회하는 non-temporal(NT) 명령.  

표 2는 key-value cache와 관련된 워크로드(랜덤 256B, 4KB load와 store)에서 Optane DC PMM의 대역폭 및 지연을 요약한다. 
나타난 대로, load에서는, regular load가 가장 높은 성능을 보였다: CPU cache prefetching은 PM 지연성을 감추고 처리량을 올리는데 효과적이다.
랜덤 워크로드에서 store의 경우, CPU cache를 우회하는 NT-store이 더 나은 성능을 보였다.
따라서, 우리는 regular load와 NT-store를 사용함으로써 최적화된 in-PM key-value cache를 사용한다.  

PM은 독특한 특성을 가지는데, 이는 multi-tenant caching에 영향을 미친다. 
예를 들어, 이전에 정의한 대로, PM의 읽기 및 쓰기 성능은 비대칭적이고, 특히 특정 크기(256B)에 대해 효율적이며, 읽기 및 쓰기에 불공정한 간섭이 존재한다.
우리가 나타낼 것은, 이러한 특성이 request regualation 수행 능력, 자원 소모량 추정 그리고 어플리케이션 감속 능력 등에 깊게 영향을 미친다는 것이다.   

#### 2.2.2 Request Regulation
이전의 Request Regulation에 대한 접근은 DRAM과 block I/O를 위해 디자인 되었다. 하지만, 어떠한 접근법도 PM에 적합하지 않다.  

메모리 요청을 제한하기 위한 기존의 기술은 clock modulation(DVFS)와 Intel Memory Bandwidth Allocation(MBA)를 사용하여 하나의 어플리케이션에 할당된 코어의 수를 조정한다.
multi-tenant caching에서, 코어의 수를 줄이는것은 적합하지 않은데, cache instance가 종종 오직 하나의 코어에 할당되기 때문이다.
Intel MBA는 메모리 트래픽을 제한하기 위해 각 코어에서 발생하는 last-level cache(LLC) miss를 관리하지만, PM과 DRAM의 miss를 구별하지 않고, 따라서 DRAM을 감속하지 않고서는 PM 접근을 엄격하게 제한할 수 없다. 
게다가, Intel MBA는 우리가 논의할 자원 소모, 간섭 그리고 어플리케이션 감속의 정보에 접근할 수 없다.
비슷하게, CPU frequency를 조정하는 것은 모든 명령에 영향을 미친다; et al) PM 트래픽 조절에 대한 CPU 주파수 조정의 비효율성을 입증했다.  

I/O 요청은 block-layer I/O 스케줄링과 함께, 소프트웨어를 통해 제한했는데, 이는 PM에 두 가지 이유로 적합하지 않다.
첫 번째, block 추상화는 bite 단위로 주소를 설정할 수 있는 PM에 상당한 read/write를 추가할 수 있다.
두 번째, merging, reordering 그리고 다른 동기화와 함께 request를 스케줄링하면 다른 low-latency PM 접근에 허용할 수 없는 오버헤드가 추가된다.  

#### 2.2.3 Resource Usage Estimation
client의 메모리 혹은 I/O 사용량을 추정하는 이전의 기술은 PM 에서 잘 동작하지 않는다.
우리는 기존의 DRAM의 I/O 사용량 추적을 위한 소프트웨어 접근법과 하드웨어 접근법을 통해 문제를 묘사한다.  

위에서 언급한 대로, CPU cache prefetching은 PM이 높은 대역폭과 낮은 지연성을 제공하는데 필수적이다. 
하지만, 소프트웨어에서 block I/O 트래픽을 추정할 때, prefetching으로 인해 발생하는 추가적인 PM 접근은 관측되지 않는다.
1KB 랜덤 read 실험을 수행할 때, 소프트웨어 수준 추정치는 60%에 불과하며, 이는 부정확한 자원 사용량 추정을 나타낸다.  

DRAM은 코어 당 메모리 컨트롤러에 대한 L3 cache line miss를 추적하기 위해 하드웨어 카운터를 사용한다.
하드웨어 카운터가 정확하게 prefetching을 측정하는 동안, 그들은 cache line 크기와 PM access 사이의 차이점을 세분화하여 측정하지 않는다.
PM은 256B의 최소 access 세분성을 갖고 있기 때문에, 64B load(하나의 L3 cache 라인)는 256B load(4개의 L3 cache 라인)와 같은 양의 PM 자원을 사용한다.
따라서, 네 cache 라인 접근은 1:4의 PMEM 접근을 초래한다.
이전의 자원 추정 시스템은 종종 자원 사용을 위한 proxy로 대역폭 소비를 사용하는 경우가 있엇지만, PM에는 적합하지 않은데 이는 비용이 접근 크기에 영향을 받고 read와 write가 다르기 때문이다.   

불행히도, 현재 PM 내부의 하드웨어 카운터 또한 적합하지 않다; 기존의 PM 카운터는 DIMM media-level에 있고, 각 client 혹은 각 코어의 사용을 추정하지 않는다.  

#### 2.2.4 Interference Estimation