// model: *
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>  // for va_list, va_start, va_end
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <gtest/gtest.h>

#define SRAND_SEED 0

void fill_wcharstring(wchar_t *str, size_t size) {
    if (size != 0) {
        for (size_t i = 0; i < size; i++) {
            str[i] =
                    L'!' + i % (L'~' - L'!');  // pick any ascii between ! and ~
        }
    }
    str[size] = L'\0';
}

void fill_randomwcharstring(wchar_t *str, size_t size) {
    if (size != 0) {
        for (size_t i = 0; i < size; i++) {
            str[i] = L'*' + rand() % (L'z' - L'*');
        }
    }
    str[size] = L'\0';
}

TEST(STRING, wcslen) {
    const wchar_t str[] = L"abcdef0123";
    int length_expected = 10;
    int length = wcslen(str);
    ASSERT_EQ(length, length_expected);
}

TEST(STRING, callocwcslen) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset;
    size_t max_length = 128;
    for (size_t alloc_length = 1; alloc_length < max_length; alloc_length++) {
        buff = (wchar_t *)calloc(sizeof(wchar_t), (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (size_t length_expected = 0; length_expected < alloc_length;
                 length_expected++) {
                fill_wcharstring(str_offset, length_expected);
                size_t length = wcslen(str_offset);
                if (length != length_expected) {
                    printf("alloc_length    = %ld\n", alloc_length);
                    printf("length_expected = %ld\n", length_expected);
                }
                ASSERT_EQ(length, length_expected);
            }
        }
        free(buff);
    }
}

TEST(STRING, callocwcslenrandomsize) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset;
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    for (size_t i = 1; i <= 100; i++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + offset + 1));
        str_offset = buff + offset;
        fill_wcharstring(str_offset, alloc_length);
        size_t length = wcslen(str_offset);
        if (length != alloc_length) {
            printf("alloc_length    = %ld\n", alloc_length);
        }
        ASSERT_EQ(length, alloc_length);
        free(buff);
    }
}

TEST(STRING, mallocwcsnlen) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset;
    size_t max_length = 128;
    for (size_t alloc_length = 1; alloc_length <= max_length; alloc_length++) {
        buff = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (size_t length_expected = 3; length_expected < alloc_length;
                 length_expected++) {
                fill_wcharstring(str_offset, length_expected);
                size_t length = wcsnlen(str_offset, (length_expected - 2));
                ASSERT_EQ(length, length_expected - 2);
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocwcsnlenrandomsize) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset;
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    for (size_t i = 1; i <= 100; i++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + offset + 1));
        str_offset = buff + offset;
        fill_wcharstring(str_offset, alloc_length);
        size_t length = wcsnlen(str_offset, (alloc_length - 2));
        ASSERT_EQ(length, alloc_length - 2);
        free(buff);
    }
}

TEST(STRING, wcscpy) {
    const wchar_t str1[] = L"abcdefgh01235678";
    int length = wcslen(str1);
    wchar_t *str2;
    str2 = (wchar_t *)malloc(sizeof(wchar_t) * length);
    wcscpy(str2, str1);
    ASSERT_STREQ(str2, str1);
    free(str2);
}

TEST(STRING, mallocwcscpy) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset, *str;
    size_t max_length = 128;
    for (size_t alloc_length = 2; alloc_length < max_length; alloc_length++) {
        buff = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (size_t length_expected = 1; length_expected < alloc_length;
                 length_expected++) {
                fill_wcharstring(str_offset, length_expected);
                str = (wchar_t *)malloc(sizeof(wchar_t) *
                                        (length_expected + 1));
                wcscpy(str, str_offset);
                ASSERT_STREQ(str, str_offset);
                free(str);
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocwcscpyrandomsize) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset, *str;
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    for (size_t i = 1; i <= 100; i++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + offset + 1));
        str_offset = buff + offset;
        fill_wcharstring(str_offset, alloc_length);
        str = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + 1));
        wcscpy(str, str_offset);
        ASSERT_STREQ(str, str_offset);
        free(str);
        free(buff);
    }
}

TEST(STRING, wcsncpy) {
    const wchar_t str1[] = L"abcdefABCDEF012356";
    int length = wcslen(str1);
    const wchar_t str_expected[] = L"abcdefABCD";
    int expstr_length = wcslen(str_expected);
    wchar_t *str2;
    str2 = (wchar_t *)malloc(sizeof(wchar_t) * length);
    wcsncpy(str2, str1, expstr_length);
    str2[expstr_length] = '\0';
    ASSERT_STREQ(str2, str_expected);
    free(str2);
}

TEST(STRING, mallocwcsncpy) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset, *str;
    size_t max_length = 128;
    for (size_t alloc_length = 2; alloc_length < max_length; alloc_length++) {
        buff = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (size_t length_expected = 1; length_expected < alloc_length;
                 length_expected++) {
                fill_wcharstring(str_offset, length_expected);
                str = (wchar_t *)malloc(sizeof(wchar_t) *
                                        (length_expected + 1));
                wcsncpy(str, str_offset, length_expected);
                str[length_expected] = '\0';
                ASSERT_STREQ(str, str_offset);
                free(str);
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocwcsncpyrandomsize) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset, *str;
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    for (size_t i = 1; i <= 100; i++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + offset + 1));
        str_offset = buff + offset;
        fill_wcharstring(str_offset, alloc_length);
        str = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + 1));
        wcsncpy(str, str_offset, alloc_length);
        str[alloc_length] = '\0';
        ASSERT_STREQ(str, str_offset);
        free(str);
        free(buff);
    }
}

TEST(STRING, wcscat) {
    const wchar_t str1[] = L"abcdefghABCDEFGH";
    int length1 = wcslen(str1);
    const wchar_t str2[] = L"0123456789";
    int length2 = wcslen(str2);
    const wchar_t str_expected[] = L"abcdefghABCDEFGH0123456789";
    int expstr_length = wcslen(str_expected);
    wchar_t *str3;
    str3 = (wchar_t *)malloc(sizeof(wchar_t) * (expstr_length * 2));
    wcscpy(str3, str1);
    wcscat(str3, str2);
    ASSERT_STREQ(str3, str_expected);
    free(str3);
}

TEST(STRING, mallocwcscat) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset, *str1, *str2;
    size_t max_length = 128;
    for (size_t alloc_length = 2; alloc_length < max_length; alloc_length++) {
        buff = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (size_t length_expected = 1; length_expected < alloc_length;
                 length_expected++) {
                fill_wcharstring(str_offset, length_expected);
                str1 = (wchar_t *)malloc(sizeof(wchar_t) *
                                         (length_expected + 1));
                fill_wcharstring(str1, length_expected);
                str2 = (wchar_t *)malloc(2 * sizeof(wchar_t) *
                                         (length_expected + 1));
                wcscpy(str2, str1);
                wcscat(str2, str_offset);
                ASSERT_STRNE(wcsstr(str2, str_offset), nullptr);
                ASSERT_STRNE(wcsstr(str2, str1), nullptr);
                free(str1);
                free(str2);
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocwcscatrandomsize) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset, *str1, *str2;
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    for (size_t i = 1; i <= 100; i++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + offset + 1));
        str_offset = buff + offset;
        fill_wcharstring(str_offset, alloc_length);
        str1 = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + 1));
        fill_wcharstring(str1, alloc_length);
        str2 = (wchar_t *)malloc(2 * sizeof(wchar_t) * (alloc_length + 1));
        wcscpy(str2, str1);
        wcscat(str2, str_offset);
        ASSERT_STRNE(wcsstr(str2, str_offset), nullptr);
        ASSERT_STRNE(wcsstr(str2, str1), nullptr);
        free(str1);
        free(str2);
        free(buff);
    }
}

TEST(STRING, wcsncat) {
    const wchar_t str1[] = L"abcdefghABCDEFGH";
    int length1 = wcslen(str1);
    const wchar_t str2[] = L"0123456789";
    int length2 = wcslen(str2);
    const wchar_t str_expected[] = L"0123456789abcdefghABCD";
    int expstr_length = wcslen(str_expected);
    wchar_t *str3;
    str3 = (wchar_t *)malloc(sizeof(wchar_t) * (expstr_length * 2));
    wcscpy(str3, str2);
    wcsncat(str3, str1, 12);
    ASSERT_STREQ(str3, str_expected);
    free(str3);
}

TEST(STRING, mallocwcsncat) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset, *str1, *str2;
    size_t max_length = 128;
    for (size_t alloc_length = 2; alloc_length < max_length; alloc_length++) {
        buff = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (size_t length_expected = 1; length_expected < alloc_length;
                 length_expected++) {
                fill_wcharstring(str_offset, length_expected);
                str1 = (wchar_t *)malloc(sizeof(wchar_t) *
                                         (length_expected + 1));
                fill_wcharstring(str1, length_expected);
                str2 = (wchar_t *)malloc(sizeof(wchar_t) *
                                         (length_expected + 1) * 2);
                wcscpy(str2, str1);
                wcsncat(str2, str_offset, length_expected);
                ASSERT_STRNE(wcsstr(str2, str_offset), nullptr);
                ASSERT_STRNE(wcsstr(str2, str1), nullptr);
                free(str1);
                free(str2);
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocwcsncatrandomsize) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset, *str1, *str2;
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    for (size_t i = 1; i <= 100; i++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + offset + 1));
        str_offset = buff + offset;
        fill_wcharstring(str_offset, alloc_length);
        str1 = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + 1));
        fill_wcharstring(str1, alloc_length);
        str2 = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + 1) * 2);
        wcscpy(str2, str1);
        wcsncat(str2, str_offset, alloc_length);
        ASSERT_STRNE(wcsstr(str2, str_offset), nullptr);
        ASSERT_STRNE(wcsstr(str2, str1), nullptr);
        free(str1);
        free(str2);
        free(buff);
    }
}

TEST(STRING, wcscmp) {
    srand(SRAND_SEED);
    wchar_t *str1;
    wchar_t *str2;
    size_t str_length = 500;
    str1 = (wchar_t *)malloc(sizeof(wchar_t) * (str_length + 1));
    fill_wcharstring(str1, str_length);
    str2 = (wchar_t *)malloc(sizeof(wchar_t) * (str_length + 1));
    wcscpy(str2, str1);
    ASSERT_EQ(wcscmp(str1, str2), 0);
    free(str1);
    free(str2);
}

TEST(STRING, mallocwcscmp) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset, *str;
    size_t max_length = 128;
    for (size_t alloc_length = 2; alloc_length < max_length; alloc_length++) {
        buff = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (size_t length_expected = 1; length_expected < alloc_length;
                 length_expected++) {
                fill_wcharstring(str_offset, length_expected);
                str = (wchar_t *)malloc(sizeof(wchar_t) *
                                        (length_expected + 1));
                wcscpy(str, str_offset);
                ASSERT_EQ(wcscmp(str, str_offset), 0);
                free(str);
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocwcscmprandomsize) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset, *str;
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    for (size_t i = 1; i <= 100; i++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + offset + 1));
        str_offset = buff + offset;
        fill_wcharstring(str_offset, alloc_length);
        str = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + 1));
        wcscpy(str, str_offset);
        ASSERT_EQ(wcscmp(str, str_offset), 0);
        free(str);
        free(buff);
    }
}

TEST(STRING, wcsncmp) {
    srand(SRAND_SEED);
    wchar_t *str1;
    wchar_t *str2;
    size_t str_length = 500;
    str1 = (wchar_t *)malloc(sizeof(wchar_t) * (1 + str_length));
    fill_wcharstring(str1, str_length);
    str2 = (wchar_t *)malloc(sizeof(wchar_t) * (1 + str_length));
    wcscpy(str2, str1);
    ASSERT_EQ(wcsncmp(str1, str2, str_length / 2), 0);
    free(str1);
    free(str2);
}

TEST(STRING, mallocwcsncmp) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset, *str;
    size_t max_length = 128;
    for (size_t alloc_length = 10; alloc_length < max_length; alloc_length++) {
        buff = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (size_t length_expected = 4; length_expected < alloc_length;
                 length_expected++) {
                fill_wcharstring(str_offset, length_expected);
                str = (wchar_t *)malloc(sizeof(wchar_t) *
                                        (length_expected + 1));
                wcscpy(str, str_offset);

                // validating all 3 cases when length is less/equal/larger than
                // the actual string length
                ASSERT_EQ(wcsncmp(str, str_offset, length_expected), 0);
                ASSERT_EQ(wcsncmp(str, str_offset, length_expected / 2), 0);
                ASSERT_EQ(wcsncmp(str, str_offset, (length_expected + 10)), 0);

                free(str);
                str = (wchar_t *)malloc(sizeof(wchar_t) *
                                        (length_expected + 1));
                fill_randomwcharstring(str, length_expected);
                ASSERT_NE(wcsncmp(str, str_offset, length_expected / 2), 0);
                free(str);
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocwcsncmprandomsize) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset, *str;
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    for (size_t i = 1; i <= 100; i++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + offset + 1));
        str_offset = buff + offset;
        fill_wcharstring(str_offset, alloc_length);
        str = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + 1));
        wcscpy(str, str_offset);

        // validating all 3 cases when length is less/equal/larger than the
        // actual string length
        ASSERT_EQ(wcsncmp(str, str_offset, alloc_length), 0);
        ASSERT_EQ(wcsncmp(str, str_offset, alloc_length / 2), 0);
        ASSERT_EQ(wcsncmp(str, str_offset, (alloc_length + 10)), 0);

        free(str);
        str = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + 1));
        fill_randomwcharstring(str, alloc_length);
        ASSERT_NE(wcsncmp(str, str_offset, alloc_length / 2), 0);
        free(str);
        free(buff);
    }
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
