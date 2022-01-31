
[[gnu::nothrow, rule]] int main()
{
    int a = 0;

    if (a)
    // the 'if' part
    {
        int b = a + 3;
    }

    return 0;
}
