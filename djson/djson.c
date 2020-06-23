#include"djson.h"
#include<math.h>

//获取内存块用于储存字符串
struct mem_list* get_mb(int size, struct mem_list* old)
{

	struct mem_list* buf = (struct mem_list*)malloc(size);
	if (buf == NULL)
		return NULL;
	buf->buf = (char*)(buf + 1);
	buf->size = size - sizeof(struct mem_list);
	if (old != NULL) {//如果有旧节点，则将新节点插入
		buf->next = old->next;
		buf->prev = old;
		old->next->prev = buf;
		old->next = buf;
	}
	else {
		buf->next = buf;
		buf->prev = buf;
	}
	return buf;
}

#define STRBUFSIZE 4096
#define ALIGN_LENGTH(size) ((size/sizeof(long)+1)*sizeof(long))

struct mem_list* strbuf = NULL;
//获取一个字符串空间
char* getstrmem(size_t len)
{
	if (strbuf == NULL) {//获取空间
		strbuf = get_mb(STRBUFSIZE, NULL);
		if (strbuf == NULL)
			return NULL;
	}
	//剩余内存不够，则重新开辟
	if (strbuf->size < len) {
		strbuf = get_mb(STRBUFSIZE, strbuf);
		if (strbuf == NULL)
			return NULL;
	}
	len = ALIGN_LENGTH(len);
	char* ret = strbuf->buf;
	strbuf->buf += len;//向后移动内存块指针
	strbuf->size -= len;
	return ret;
}

#define JSONSTRUCTMEMSIZE 4096
struct mem_list* jsonmem = NULL;
void* getjsonmem(size_t size)
{
	if (jsonmem == NULL) {
		jsonmem = get_mb(JSONSTRUCTMEMSIZE, NULL);
		if (jsonmem == NULL)
			return NULL;
	}
	//剩余内存不够，则重新开辟
	if (jsonmem->size < size) {
		jsonmem = get_mb(STRBUFSIZE, jsonmem);
		if (jsonmem == NULL)
			return NULL;
	}
	size = ALIGN_LENGTH(size);
	char* ret = jsonmem->buf;
	jsonmem->buf += size;//向后移动内存块指针
	jsonmem->size -= size;
	memset(ret, 0, size);
	return ret;
}

//初始化djson
int djson_init()
{
	strbuf = get_mb(STRBUFSIZE, NULL);
	if (strbuf == NULL)
		return -1;
	jsonmem = get_mb(JSONSTRUCTMEMSIZE, NULL);
	if (jsonmem == NULL)
		return -1;
	return 0;
}

//key复制
char* keycpy(const char* src, size_t* len)
{
	size_t i = 0, n = 0;
	*len = 0;
	char* key = strbuf->buf;
	size_t size = strbuf->size;
	while (src[i] != '"' && src[i]) {
		if (strbuf->size <= 0) {//空间不足
			//还原
			strbuf->buf = key;
			strbuf->size = size;
			//重新获取空间
			struct mem_list* newstrbuf = get_mb(STRBUFSIZE, strbuf);
			if (newstrbuf == NULL) {
				return 0;
			}
			strbuf = newstrbuf;
			i = 0; n = 0;
			key = strbuf->buf;
			size = strbuf->size;
		}
		strbuf->buf[n++] = src[i++];
	}
	if (!src[i]) {
		return NULL;
	}
	strbuf->buf[n++] = 0;
	strbuf->buf[n++] = 0;
	strbuf->buf[n++] = 0;
	strbuf->buf[n++] = 0;

	strbuf->buf += n;
	strbuf->size -= n;
	*len = n - 4;
	return key;
}

//为指定对象添加项
int insert_obj(struct jo* obj, void* a, json_st type)
{
	if (obj == NULL || a == NULL)
		return -1;
	((struct js*)a)->next = obj->value;
	((struct js*)a)->nexttype = obj->value_t;
	((struct js*)a)->prev = ((struct js*)(obj->value))->prev;
	((struct js*)a)->prevtype = ((struct js*)(obj->value))->prevtype;

	((struct js*)((struct js*)(obj->value))->prev)->next = a;
	((struct js*)((struct js*)(obj->value))->prev)->nexttype = type;
	((struct js*)(obj->value))->prev = a;
	((struct js*)(obj->value))->prevtype = type;

	obj->valuenum++;
	return obj->valuenum;
}

//将字符串转换为数字
double djson_atof(const char* str, size_t* len)
{
	int i = 0;
	char sss[50];
	if (str[i] == '-') {
		i++;
		sss[i] = str[i];
	}
	while ((str[i] >= '0' && str[i] <= '9') || str[i] == '.') {
		sss[i] = str[i];
		i++;
	}
	sss[i] = '\0';
	*len = i;
	return atoi(sss);
}
//字符串转换为布尔型
//此函数没有判断格式是否正确
json_bool djson_atob(const char* str, size_t* len)
{
	int i = 4;
	json_bool bl = JSON_BOOL_FLASE;
	if (strncmp(str, "true", i) == 0) {
		bl = JSON_BOOL_TRUE;
	}
	else {
		if (strncmp(str, "flase", ++i) != 0) {
			bl = -1;
		}
	}
	//判断是否符合格式
	while (str[i] == 0x20 || str[i] == '\t' || str[i] == '\n') {
		i++;
	}
	if (str[i] != ',' && str[i] != '}' && str[i] != ']')
		bl = -1;
	*len = i;
	return bl;
}

#define JSON_MAX_KEY_LENGTH 50	//key字符串最长长度
//解析json对象，返回一个djson指针
struct jo* djson_obj(char* str, int* num, struct jo* obj)
{
	enum djson_jo_state {
		djson_obj_st_object = 1,
		djson_obj_st_string,
		djson_obj_st_number,
		djson_obj_st_array,
		djson_obj_st_bool,
		djson_obj_st_null,
		djson_obj_run_idle,//跳过字符
		djson_obj_run_readend,//value读取结束
		djson_obj_run_type,	//判断类型中
		djson_obj_run_valueend//value读取完成
	};
	int i = 0;
	int state = djson_obj_run_idle;
	char* key = NULL;//只想key字符串
	size_t length = 0;
	//申请一个空间用于储存字符串

	struct jo* newobj = NULL;
	struct jo* newjo = NULL;
	struct js* newjs = NULL;
	struct ja* newja = NULL;
	struct jn* newjn = NULL;
	struct jb* newjb = NULL;
	struct je* newje = NULL;

	int objlen = 0;
	//djson_jo状态

	while (str[i]) {
		switch (state) {//处理运行状态

		case djson_obj_st_object: {//对象
			newobj = (struct jo*)getjsonmem(sizeof(struct jo));
			//newobj = djson_obj(&(str[i]),&objlen,obj);
			if (newobj == NULL)
				return NULL;
			newobj->key = keycpy(key, &(newobj->keylen));
			if (newobj == NULL)
				return NULL;
			newobj = djson_obj(&(str[i]), &objlen, newobj);
			newobj->type = djson_object;
			if (obj->value == NULL) {//说明这是第一个
				obj->value = (void*)newobj;
				obj->value_t = djson_object;
				newobj->prev = (void*)newobj;
				newobj->prevtype = djson_object;
				newobj->next = (void*)newobj;
				newobj->nexttype = djson_object;
			}
			else {//加入链表
				insert_obj(obj, newobj, djson_object);
			}
			state = djson_obj_run_idle;//value读取结束
			i += objlen;
		}break;

		case djson_obj_st_string: {//字符串,此时的&str[i]指向字符串
			newjs = (struct js*)getjsonmem(sizeof(struct js));
			if (newjs == NULL)
				return NULL;
			newjs->key = keycpy(key, &length);
			newjs->keylen = length;
			newjs->str = keycpy(&(str[i]), &length);
			newjs->strlen = length;
			newjs->type = djson_string;
			i += (length + 1);
			//printf(newjs->str);
			//将此结构添加到同一级结构中
			if (obj->value == NULL) {//说明这是第一个
				obj->value = (void*)newjs;
				obj->value_t = djson_string;
				newjs->prev = (void*)newjs;
				newjs->prevtype = djson_string;
				newjs->next = (void*)newjs;
				newjs->nexttype = djson_string;
			}
			else {//加入链表
				insert_obj(obj, newjs, djson_string);
			}
			state = djson_obj_run_idle;//value读取结束
		}break;

		case djson_obj_st_number: {//数字
			newjn = (struct jn*)getjsonmem(sizeof(struct jn));
			if (newjn == NULL)
				return NULL;
			newjn->type = djson_number;
			newjn->key = keycpy(key, &(newjn->keylen));
			if (newjn->key == NULL)
				return NULL;
			//获取值
			newjn->num = djson_atof(&str[i], &length);
			i += length;

			if (obj->value == NULL) {//说明这是第一个
				obj->value = (void*)newjn;
				obj->value_t = djson_number;
				newjn->prev = (void*)newjn;
				newjn->prevtype = djson_number;
				newjn->next = (void*)newjn;
				newjn->nexttype = djson_number;
			}
			else {//加入链表
				insert_obj(obj, newjn, djson_number);
			}
			state = djson_obj_run_idle;//value读取结束
		}break;

		case djson_obj_st_array: {//数组
			newja = (struct ja*)getjsonmem(sizeof(struct ja));
			if (newja == NULL)
				return NULL;
			newja->type = djson_array;
			newja->key = keycpy(key, &(newja->keylen));
			if (newja->key == NULL)
				return NULL;

		}break;

		case djson_obj_st_bool: {//布尔
			newjb = (struct jb*)getjsonmem(sizeof(struct jb));
			if (newjb == NULL) {
				return NULL;
			}
			newjb->type = djson_bool;
			newjb->key = keycpy(key, &(newjb->keylen));
			if (newjb->key == NULL)
				return NULL;
			newjb->bol = djson_atob(&(str[i]), &length);
			if (newjb->bol == -1) {//格式错误
				return NULL;
			}
			i += length;
			if (obj->value == NULL) {//说明这是第一个
				obj->value = (void*)newjb;
				obj->value_t = djson_bool;
				newjb->prev = (void*)newjb;
				newjb->prevtype = djson_bool;
				newjb->next = (void*)newjb;
				newjb->nexttype = djson_bool;
			}
			else {//加入链表
				insert_obj(obj, newjb, djson_bool);
			}
			state = djson_obj_run_idle;//value读取结束
		}break;

		case djson_obj_st_null: {
			newje = (struct je*)getjsonmem(sizeof(struct je));
			if (newje == NULL)
				return NULL;
			newje->type = djson_null;
			newje->key = keycpy(key, &(newje->keylen));
			if (newje == NULL)
				return NULL;
			i += 4;//跳过null字符串
			if (obj->value == NULL) {//说明这是第一个
				obj->value = (void*)newje;
				obj->value_t = djson_null;
				newje->prev = (void*)newje;
				newje->prevtype = djson_null;
				newje->next = (void*)newje;
				newje->nexttype = djson_null;
			}
			else {//加入链表
				insert_obj(obj, newje, djson_null);
			}
			state = djson_obj_run_idle;//value读取结束
		}break;

		}
		//判断数字
		if ((str[i] > '0' && str[i] < '9') || str[i] == '-') {//数字
			state = djson_obj_st_number;
			continue;
		}
		//判断字符
		switch (str[i]) {

		case '{': {
			i++;
			state = djson_obj_st_object;

		}break;
		case '}': {
			*num = (i + 1);
			return obj;
		}break;
		case '[': {//数组
			i++;
			state = djson_obj_st_array;
		}break;
		case ']': {

		}break;
		case '"': {//字符串
			i++;
			if (state == djson_obj_run_idle) {//表示当前是在读取key
				key = &str[i];
				//跳过key
				while (str[i] != ':' && str[i]) { i++; }
				if (str[i] == 0)
					return NULL;
				state = djson_obj_run_type;//表示正在判断类型
			}
			else {
				state = djson_obj_st_string;//字符串
			}
		}break;
		case 't'://布尔值
		case 'f': {
			state = djson_obj_st_bool;
		}break;
		case 'n': {
			state = djson_obj_st_null;
		}break;
		case ',': {
			if (state == djson_obj_run_valueend) {//表示上一个值读取完成，将所有状态置位
				state = djson_obj_run_idle;
			}
			i++;
		}break;
		default:i++;
		}
	}
	return NULL;
}

void printjsonobj(struct jo* obj)
{
	int n = 0;
	if (obj == NULL)
		return;
	printf("%s:{\n", obj->key);
	void* a = obj->value;
	void* temp = a;
	json_st type = obj->value_t;
	while (a) {
		switch (type) {
		case djson_object: {
			//printf("\"%s\":", ((struct jo*)a)->key);
			printjsonobj((struct jo*)a);
		}break;
		case djson_string: {
			printf("\"%s\":", ((struct js*)a)->key);
			printf("\"%s\"", ((struct js*)a)->str);
		}break;
		case djson_number: {
			printf("\"%s\":", ((struct jn*)a)->key);
			printf("%f", ((struct jn*)a)->num);
		}break;
		case djson_bool: {
			printf("\"%s\":", ((struct jb*)a)->key);
			printf("%s", ((struct jb*)a)->bol == JSON_BOOL_TRUE ? "true" : "flase");
		}break;
		case djson_null: {
			printf("\"%s\":", ((struct je*)a)->key);
			printf("null");
		}break;
		}
		if (((struct js*)a)->next == temp) {
			a = NULL;
		}
		else {
			type = ((struct js*)a)->nexttype;
			a = ((struct js*)a)->next;
			printf(",\n");
		}
	}
	printf("\n}\n");
}
