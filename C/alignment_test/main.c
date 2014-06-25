#include <stdio.h>
#include <stddef.h>

#define TYPE_ALIGNMENT(type) offsetof(struct {char x; type t;}, t)
#define PRINT_ALIGNMENT(type) printf("TYPE_ALIGNMENT("#type")=%d\n", TYPE_ALIGNMENT(type));

int main() {
	PRINT_ALIGNMENT(char);
	PRINT_ALIGNMENT(int);
	PRINT_ALIGNMENT(long);
	PRINT_ALIGNMENT(long long);
	PRINT_ALIGNMENT(struct{int i; char c;});
	return 0;
}