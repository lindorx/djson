#pragma once
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

typedef char json_bool;
#define JSON_BOOL_FLASE 0
#define JSON_BOOL_TRUE 1

//解析json
typedef enum json_st {
	djson_string = 1,
	djson_number,
	djson_object,
	djson_array,
	djson_bool,
	djson_null
}json_st;

struct js {//json字符串
	//环链
	void* next;
	json_st nexttype;
	void* prev;
	json_st prevtype;

	json_st type;//此结构类型
	char* key;
	size_t keylen;
	char* str;
	size_t strlen;
};

struct jn {//数字
	//环链
	void* next;
	json_st nexttype;
	void* prev;
	json_st prevtype;

	json_st type;//此结构类型
	char* key;
	size_t keylen;
	double num;

};

struct ja {//数组
	//环链
	void* next;
	json_st nexttype;
	void* prev;
	json_st prevtype;

	json_st type;//此结构类型
	char* key;
	size_t keylen;
	void* array;//数组指针
	size_t arraylen;//数组元素数量
};

struct jb {//布尔
		//环链
	void* next;
	json_st nexttype;
	void* prev;
	json_st prevtype;

	json_st type;//此结构类型
	char* key;
	size_t keylen;
	char bol;
};

struct je {//null空
	//环链
	void* next;
	json_st nexttype;
	void* prev;
	json_st prevtype;

	json_st type;//此结构类型
	char* key;
	size_t keylen;
};

struct jo {//json对象
	//环链
	void* next;
	json_st nexttype;
	void* prev;
	json_st prevtype;

	json_st type;//此结构类型
	char* key;//此对象的key
	size_t keylen;

	void* value;//指向包含的项
	json_st value_t;//项类型
	int valuenum;
};

//类型指针联合体
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
	size_t size;//剩余大小
};

int djson_init();

//获取内存块用于储存字符串
struct mem_list* get_mb(int size, struct mem_list* old);

char* getstrmem(size_t len);

void* getjsonmem(size_t size);

char* keycpy(const char* src, size_t* len);

int insert_obj(struct jo* obj, void* a, json_st type);

//将字符串转换为数字
double djson_atof(const char* str, size_t* len);

//字符串转换为布尔型
json_bool djson_atob(const char* str, size_t* len);

//解析json对象，返回一个djson指针
struct jo* djson_obj( char* str, int* num, struct jo* obj);

void printjsonobj(struct jo* obj);