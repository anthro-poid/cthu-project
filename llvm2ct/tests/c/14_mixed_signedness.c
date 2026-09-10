// EXPECT: 16
int main()
{
    int a = 5;
    int sa = a + a;

    unsigned int b = 3;
    unsigned int sb = b + b;

// This is acutally questionable. LLVM IR uses i32 add, but in Cthu
// it results in i32, which does not preserve C semantics, but preserve
// LLVM semantics.
    return sa + sb;
}
