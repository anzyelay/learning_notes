/**
这行代码使用了GCC的一个特性，即attribute，它可以指定函数或变量的特定属性，以此来影响编译器的行为。在这个例子中，我们指定了一个cleanup属性，用于指定一个函数在变量离开作用域时自动调用以释放其内存。

具体来说，这行代码定义了一个名为charp的char*类型的变量，并将其与cleanup属性相关联。当变量charp离开其作用域(例如，当其在函数中定义的时候，函数返回或者当一个代码块执行完毕时)时，编译器将自动调用一个名为free_charp的函数，用于释放charp所指向的内存。

请注意，这需要确保在变量离开作用域之前，charp指向的内存不被重写或释放，否则可能会导致未定
 */
static void
free_charp(char **ptr)
{
	free(*ptr);
}

#define autochar __attribute__((__cleanup__(free_charp))) char

void test()
{
	autochar *charp = alloc(1, 1024);
	// do something here
}
