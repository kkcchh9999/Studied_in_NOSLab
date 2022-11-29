#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//명령어를 저장할 구조체  
typedef struct Line1 {
	
	struct Line1* next;
	char first[20];
	char second[20];
	char third[20];
	int addr;

} Line;

//Symbol Table을 구성할 구조체
typedef struct Symbol1 {
	
	struct Symbol1* next;
	char symbol[20];
	int addr;
} Symbol;

//함수 선언
Line* add_line(Line* head, char* line);
Line* file_read(char* file_name, Line* head);
Line* calculate_addr(Line* head);
Symbol* add_symbol(Symbol* symbol_table, Line* head);
void print_input(Line* head);
void print_symbol(Symbol* symbolTable);
void print_intermed_file(Line* head);
void print_object(Line* head, Symbol* symbolTable, char mnemonicl[][5], char codel[][3]);
Symbol* remove_node(Symbol* symbolTable, Symbol* target);


int main(int argc, char* argv[]) {
	
	//링크드 리스트의 head
	Line* head = NULL;
	Symbol* symbolTable = NULL;

	//변수
	char opcode[10], oprand[10], label[10], a[10], ad[10], symbol[10], ch;
	char code [10][10], codel[4][3] = {"33", "44", "53", "57"};
	char mnemonic[6][6] = {"START", "LDA", "STA", "LDCH", "STCH", "END"}; 
	char mnemonicl[4][5] = {"LDA", "STA", "LDCH", "STCH"}; 
	char file_name[50];

	//옵션 읽기 
    int option;
    while ((option = getopt(argc, argv, "i:")) != -1) {
        switch(option) {
            case 'i':
                printf("입력 파일명: %s\n", optarg);
				strcpy(file_name, optarg);
                break;
			case '?': 
				printf("잘못된 옵션 %c\n", optopt);
				return 0;
				break;
        }
    }

	//파일 읽기 
	head = file_read(file_name, head);
	if (head == NULL) return 0;

	//입력파일 출력
	print_input(head);

	//주소 계산 
	head = calculate_addr(head);
	
	//심볼테이블 생성, 출력
	symbolTable = add_symbol(symbolTable, head);
	print_symbol(symbolTable);

	//intermed file 출력 
	print_intermed_file(head);

	//object 프로그램 출력
	print_object(head, symbolTable, mnemonicl, codel);

	return 0;
}

//노드 생성 함수
Line* add_line(Line* head,char* line) {

	//새로운 노드 생성 
	Line* tmp = (Line*)malloc(sizeof(Line));

	//입력받은 문자열 공백 기준으로 잘라서 노드에 삽입 
	char *words[3] = { NULL, }; 
	char cp[60];
	strcpy(cp, line);

	char *ptr = strtok(cp, " "); 
	words[0] = ptr; 
	ptr = strtok(NULL, " ");
	words[1] = ptr;
	ptr = strtok(NULL, " ");
	words[2] = ptr; 
	
	strcpy(tmp->first, words[0]);
	strcpy(tmp->second, words[1]);
	strcpy(tmp->third, words[2]);
	tmp->next = NULL;

	//노드가 없을 경우 
	if(head == NULL) {
		head = tmp;
		return head;
	} else {	//노드가 존재할 경우 마지막 노드 탐색 
		Line* tail = head;
		while (tail->next != NULL){
			tail = tail->next;
		}
		//마지막 노드 뒤에 노드 추가 
		tail->next = tmp;
		return head;
	}
}

//입력 파일을 출력하는 함수
void print_input(Line* head) {
 
	printf("\n\nThe contents of Input File:\n\n");

	//링크드 리스트 순회하며 출력
	Line *tmp = head;
	while(tmp != NULL) {
		printf("%s %s %s\n", tmp->first, tmp->second, tmp->third);
		tmp = tmp->next;
	}
}

// 파일 읽기 함수
Line* file_read(char* file_name, Line* head) {

	char line[60]; 	
	FILE* fp = NULL;
	//파일 열기
	fp = fopen(file_name, "r");
	//예외처리
	if (fp == NULL) {
		printf("%s 이름의 파일이 없음\n", file_name);
		return NULL;
	}

	//한줄씩 읽기 
	while (fgets(line, sizeof(line), fp) != NULL) {
		//개행 제거  
		line[strlen(line)-1] = '\0'; 
		//읽은 라인으로 노드 추가
		head = add_line(head, line);
	}
	fclose(fp);
	return head;
}

//주소값 계산 
Line* calculate_addr(Line* head) {

	Line* tmp = head;	
	int count = 0; 
	int cur = atoi(head->third);
	//링크드 리스트 순회
	while(tmp != NULL) {
		if ((strcmp(tmp->second, "START")) == 0) { //start 명령어로부터 다음 라인의 주소 할당
			tmp->next->addr = cur;
		} else if ((strcmp(tmp->second, "END")) == 0) {	//end 명령어
		
		} else if ((strcmp(tmp->second, "BYTE")) == 0) {	//이외에 byte 혹은 word 크기만큼 주소 계산 
			count += 1;
			tmp->next->addr = cur+1;
			cur += 1;
		} else if ((strcmp(tmp->second, "RESW")) == 0) { 
			int add = atoi(tmp->third) * 3;
			count += add;
			tmp->next->addr = cur + add;
			cur += add;
		} else if ((strcmp(tmp->second, "RESB")) == 0) {
			int add = atoi(tmp->third);
			count += add;
			tmp->next->addr = cur + add;
			cur += add;
		} else {
			count += 3;
			tmp->next->addr = cur+3; 
			cur += 3;
		}
		tmp = tmp->next;
	}	

	printf("\n\nLength of the input program is %d.\n", count);
	return head;
}

//심볼 테이블 생성
Symbol* add_symbol(Symbol* symbolTable, Line* head) {

	Line* tmp = head;
	//링크드 리스트 순회 
	while (tmp != NULL) {
		if ((strcmp(tmp->first, "**")) != 0 && (strcmp(tmp->second, "START")) != 0) {
			//심볼 추가 
			Symbol* stmp = (Symbol*)malloc(sizeof(Symbol));
			stmp->next = NULL;
			stmp->addr = tmp->addr;
			strcpy(stmp->symbol, tmp->first);

			//심볼이 없을때
			if (symbolTable == NULL) {
				symbolTable = stmp;
			} else {	//심볼이 있을 때 
				Symbol* tail = symbolTable;
				while (tail->next != NULL) {
					tail = tail->next;
				}
				tail->next = stmp;
			}
		}
		tmp = tmp->next;
	}	
	return symbolTable;	
}

//심볼테이블 출력
void print_symbol(Symbol* symbolTable) {

	Symbol* tmp = symbolTable;
	printf("\nThe contents of Symbol Table:\n\n");

	while(tmp != NULL) {
		printf("%s %d\n", tmp->symbol, tmp->addr);
		tmp = tmp->next;
	}
}

//intermed file 출력
void print_intermed_file(Line* head) {

	Line* tmp = head;
	printf("\n\nThe contents of Intermed File: \n\n");

	while(tmp != NULL) {
		if ((strcmp(tmp->first, "COPY")) == 0) {
			printf("%s %s %s\n", tmp->first, tmp->second, tmp->third);
		} else {
			printf("%d %s %s %s\n", tmp->addr, tmp->first, tmp->second, tmp->third);
		}
		tmp = tmp->next;
	}
}

//object 출력
void print_object(Line* head, Symbol* symbolTable, char mnemonicl[][5], char codel[][3]) {

	printf("\n\nObject Program has been generated.\n\nObject Program:\n\n");
//////////////H 출력
	Line* tmp = head;
	printf("H^%s", tmp->first);
	while (tmp != NULL) {
		//시작 주소 
		if ((strcmp(tmp->second, "START") == 0)) {
			printf("^00%d", tmp->next->addr);
		}
		//종료 주소
		if ((strcmp(tmp->second, "END") == 0)){
			printf("^00%d\n", tmp->addr);
		}
		tmp = tmp->next;
	}
//////////////T 출력
	tmp = head;
	//시작 주소 
	printf("T^00%d",tmp->next->addr);
	//코드 길이
	int start = tmp->next->addr;
	while (tmp->next != NULL){
		tmp = tmp->next;
	} 
	int end = tmp->addr;
	printf("^%d", end-start-1);

	//명령
	tmp = head;
	while(tmp != NULL) {
		Symbol* stmp = symbolTable;

		//LDA
		if ((strcmp(tmp->second,mnemonicl[0])) == 0) {
			printf("^%s", codel[0]);
			
			while(stmp != NULL) {
				if((strcmp(stmp->symbol, tmp->third)) == 0) {
					printf("%d", stmp->addr);
					symbolTable = remove_node(symbolTable, stmp);
					break;
				}
				stmp = stmp->next;
			}			
		}
		//STA
		if ((strcmp(tmp->second,mnemonicl[1])) == 0) {
			printf("^%s", codel[1]);
			while(stmp != NULL) {
				if((strcmp(stmp->symbol, tmp->third)) == 0) {
					printf("%d", stmp->addr);
					symbolTable = remove_node(symbolTable, stmp);
					break;
				}
				stmp = stmp->next;
			}
		}
		//LDCH
		if ((strcmp(tmp->second,mnemonicl[2])) == 0) {
			printf("^%s", codel[2]);
			while(stmp != NULL) {
				if((strcmp(stmp->symbol, tmp->third)) == 0) {
					printf("%d", stmp->addr);
					symbolTable = remove_node(symbolTable, stmp);
					break;
				}
				stmp = stmp->next;
			}
		}
		//STCH
		if ((strcmp(tmp->second,mnemonicl[3])) == 0) {
			printf("^%s", codel[3]);
			while(stmp != NULL) {
				if((strcmp(stmp->symbol, tmp->third)) == 0) {
					printf("%d", stmp->addr);
					symbolTable = remove_node(symbolTable, stmp);
					break;
				}
				stmp = stmp->next;
			}
		}
		//WORD
		if ((strcmp(tmp->second, "WORD")) == 0) {
			printf("^%06d", atoi(tmp->third));
		}
		//BYTE
		if ((strcmp(tmp->second, "BYTE")) == 0) {
			char word[20];
			strcpy(word, tmp->third);
			char* ptr = strtok(word, "'");
			ptr = strtok(NULL, "'");
            printf("^");

			if (word[0] == 67) {	//char
				for (int i = 0; i < strlen(ptr); i ++) {
					printf("%0x", ptr[i]);
				}
			} else {	//16진수 상수 
				printf("%s", ptr);
			}	
		}
		tmp = tmp->next;
	}
//////////////E 출력 
	tmp = head;
	printf("\nE^00%d\n", tmp->next->addr); 
}
//Symbol Table 제거 
Symbol* remove_node(Symbol* symbolTable, Symbol* target) {
	//목표가 head일때
	//
	if (target->addr == symbolTable->addr && (strcmp(target->symbol, symbolTable->symbol)==0)) { 
		symbolTable = symbolTable->next;
		return symbolTable;
	} 
  	//이외의 경우  
	else {  
		Symbol* tmp = symbolTable;
		while(tmp->next != target) {
			tmp = tmp->next;
		}
		tmp->next = target->next;
		free(target);
		return symbolTable;
	}
}
