
[[gnu::nothrow, rule]] int main()
{
    int a = 0;

    if (a)
    {
        int b = a + 3;
    }

    return 0;
}
