#include<stdlib.h>
#include<stdio.h>
#include"djson.h"

int main(int argc, char** argv)
{
	djson_init();
	FILE* f;
	fopen_s(&f, "C:\\Users\\Admin\\Desktop\\test.json", "r");
	if (f == NULL)return 0;
	char* buf = (char*)malloc(4096);
	memset(buf, 0, 4096);
	fread_s(buf, 4096, 4096, 1, f);

	struct jo obj = { 0 };
	char sss[] = "hello\"";
	int keylen;
	obj.key = keycpy(sss, &keylen);
	obj.keylen = keylen;
	int n;
	struct jo* j = djson_obj(buf + 1, &n, &obj);
	printjsonobj(j);
	return 0;
}