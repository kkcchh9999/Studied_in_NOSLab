#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct Line1 {
	
	struct Line1* next;
	char first[20];
	char second[20];
	char third[20];
	int addr;

} Line;

typedef struct Symbol1 {
	
	struct Symbol1* next;
	char symbol[20];
	int addr;
} Symbol;


Line* add_line(Line* head, char* line);
Line* file_read(char* file_name, Line* head);
Line* calculate_addr(Line* head);
Symbol* add_symbol(Symbol* symbol_table, Line* head);
void print_input(Line* head);
void print_symbol(Symbol* symbolTable);
void print_intermed_file(Line* head);
void print_object(Line* head);

int main() {
	
	//링크드 리스트 시작 
	Line* head = NULL;
	Symbol* symbolTable = NULL;

	//저장할 변수
	char opcode[10], oprand[10], label[10], a[10], ad[10], symbol[10], ch;
	char code [10][10], codel[4][3] = {"33", "44", "53", "57"};
	char mnemonic[6][6] = {"START", "LDA", "STA", "LDCH", "STCH", "END"}; 
	char mnemonicl[4][5] = {"LDA", "STA", "LDCH", "STCH"}; 

	char file_name[20];

	printf("파일 이름을 입력하세요: ");
	scanf("%s", file_name);

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

	//object 출력
	print_object(head); 

	return 0;
}

//노드 생성 함수
Line* add_line(Line* head,char* line) {

	//새로운 노드 생성 
	Line* tmp = (Line*)malloc(sizeof(Line));

	//문자열 잘라서 노드에 삽입 
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
	} else {	//노드가 존재할 경우 
			//마지막 노드 탐색 
		Line* tail = head;
		while (tail->next != NULL){
			tail = tail->next;
		}
		//마지막 노드 뒤에 노드 추가 
		tail->next = tmp;
		return head;
	}
}

//링크드 리스트 출력 테스트 함수 
void print_input(Line* head) {
 

	printf("\n\nThe contents of Input File:\n\n");

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
		//노드 추가
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
	
	while(tmp != NULL) {

		if ((strcmp(tmp->first, "COPY")) == 0) {
			tmp->next->addr = cur;
		} else if ((strcmp(tmp->second, "END")) == 0) {
			
		} else if ((strcmp(tmp->second, "BYTE")) == 0 || (strcmp(tmp->second, "RESB")) == 0) {
			count += 1;
			tmp->next->addr = cur+1;
			cur += 1;
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

Symbol* add_symbol(Symbol* symbolTable, Line* head) {

	Line* tmp = head;

	while (tmp != NULL) {
		if ((strcmp(tmp->first, "**")) != 0 && (strcmp(tmp->first, "COPY")) != 0) {
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

void print_symbol(Symbol* symbolTable) {

	Symbol* tmp = symbolTable;
	printf("\nThe contents of Symbol Table:\n\n");

	while(tmp != NULL) {
		printf("%s %i\n", tmp->symbol, tmp->addr);
		tmp = tmp->next;
	}
}

void print_intermed_file(Line* head) {

	Line* tmp = head;
	printf("\n\nThe contents of Intermed File: \n\n");

	while(tmp != NULL) {
		if ((strcmp(tmp->first, "COPY")) == 0) {
			printf("%s %s %s\n", tmp->first, tmp->second, tmp->third);
		} else {
			printf("%i %s %s %s\n", tmp->addr, tmp->first, tmp->second, tmp->third);
		}
		tmp = tmp->next;
	}
}

void print_object(Line* head) {

	printf("\n\nObject Program has been generated.\n\nObject Program:\n\n");

	//H 출력 
	Line* tmp = head;
	printf("H^%s", tmp->first);
	while (tmp != NULL) {
		if ((strcmp(tmp->second, "START") == 0)) {
			printf("^00%d", tmp->next->addr);		
		}

		if ((strcmp(tmp->second, "END") == 0)){
			printf("^00%d\n", tmp->addr); 	
		}
		tmp = tmp->next;
	}
}
