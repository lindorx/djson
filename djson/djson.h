#pragma once
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

typedef char json_bool;
#define JSON_BOOL_FLASE 0
#define JSON_BOOL_TRUE 1

//����json
typedef enum json_st {
	djson_string = 1,
	djson_number,
	djson_object,
	djson_array,
	djson_bool,
	djson_null
}json_st;

struct js {//json�ַ���
	//����
	void* next;
	json_st nexttype;
	void* prev;
	json_st prevtype;

	json_st type;//�˽ṹ����
	char* key;
	size_t keylen;
	char* str;
	size_t strlen;
};

struct jn {//����
	//����
	void* next;
	json_st nexttype;
	void* prev;
	json_st prevtype;

	json_st type;//�˽ṹ����
	char* key;
	size_t keylen;
	double num;

};

struct ja {//����
	//����
	void* next;
	json_st nexttype;
	void* prev;
	json_st prevtype;

	json_st type;//�˽ṹ����
	char* key;
	size_t keylen;
	void* array;//����ָ��
	size_t arraylen;//����Ԫ������
};

struct jb {//����
		//����
	void* next;
	json_st nexttype;
	void* prev;
	json_st prevtype;

	json_st type;//�˽ṹ����
	char* key;
	size_t keylen;
	char bol;
};

struct je {//null��
	//����
	void* next;
	json_st nexttype;
	void* prev;
	json_st prevtype;

	json_st type;//�˽ṹ����
	char* key;
	size_t keylen;
};

struct jo {//json����
	//����
	void* next;
	json_st nexttype;
	void* prev;
	json_st prevtype;

	json_st type;//�˽ṹ����
	char* key;//�˶����key
	size_t keylen;

	void* value;//ָ���������
	json_st value_t;//������
	int valuenum;
};

//����ָ��������
typedef union jt_t {
	struct jo* obj;
	struct js* str;
	struct jn* num;
	struct ja* ary;
	struct jb* bol;
	struct je* nul;
}jt_t;

struct mem_list {
	char* buf;
	struct mem_list* next;
	struct mem_list* prev;
	size_t size;//ʣ���С
};

int djson_init();

//��ȡ�ڴ�����ڴ����ַ���
struct mem_list* get_mb(int size, struct mem_list* old);

char* getstrmem(size_t len);

void* getjsonmem(size_t size);

char* keycpy(const char* src, size_t* len);

int insert_obj(struct jo* obj, void* a, json_st type);

//���ַ���ת��Ϊ����
double djson_atof(const char* str, size_t* len);

//�ַ���ת��Ϊ������
json_bool djson_atob(const char* str, size_t* len);

//����json���󣬷���һ��djsonָ��
struct jo* djson_obj( char* str, int* num, struct jo* obj);

void printjsonobj(struct jo* obj);