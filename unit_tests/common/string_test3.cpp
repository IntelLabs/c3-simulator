// model: *
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>  // for va_list, va_start, va_end
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <gtest/gtest.h>

#define SRAND_SEED 0

void fill_string(char *str, size_t size) {
    if (size != 0) {
        for (size_t i = 0; i < size; i++) {
            str[i] = '!' + i % ('~' - '!');  // pick any ascii between ! and ~
        }
    }
    str[size] = '\0';
}

void fill_randomstring(char *str, size_t size) {
    if (size != 0) {
        for (size_t i = 0; i < size; i++) {
            str[i] = '*' + rand() % ('z' - '*');
        }
    }
    str[size] = '\0';
}

TEST(STRING, strcasecmp) {
    const char str1[] = "ABCDEFrtyhjfGHACBkleqdgbmcKLQSWTXYZ";
    char *str2;
    size_t str_length = strlen(str1);
    str2 = (char *)malloc(sizeof(char) * str_length);
    strcpy(str2, str1);
    char *str3 = str2;
    while (*str3) {
        *str3 = toupper(*str3);
        str3++;
    }
    ASSERT_EQ(strcasecmp(str1, str2), 0);
    free(str2);
}

TEST(STRING, mallocstrcasecmp) {
    srand(SRAND_SEED);
    char *buff, *str_offset, *str;
    size_t max_length = 128;
    for (size_t alloc_length = 2; alloc_length < max_length; alloc_length++) {
        buff = (char *)malloc(sizeof(char) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (size_t length_expected = 1; length_expected < alloc_length;
                 length_expected++) {
                fill_string(str_offset, length_expected);
                str = (char *)malloc(sizeof(char) * (length_expected + 1));
                strcpy(str, str_offset);
                char *temp = str;
                while (*temp) {
                    *temp = toupper(*temp);
                    temp++;
                }
                ASSERT_EQ(strcasecmp(str, str_offset), 0);
                free(str);
                str = (char *)malloc(sizeof(char) * (length_expected + 1));
                fill_randomstring(str, length_expected);
                temp = str;
                while (*temp) {
                    *temp = toupper(*temp);
                    temp++;
                }
                ASSERT_NE(strcasecmp(str, str_offset), 0);
                free(str);
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocstrcasecmprandomsize) {
    srand(SRAND_SEED);
    char *buff, *str_offset, *str;
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    for (size_t i = 1; i <= 100; i++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (char *)malloc(sizeof(char) * (alloc_length + offset + 1));
        str_offset = buff + offset;
        fill_string(str_offset, alloc_length);
        str = (char *)malloc(sizeof(char) * (alloc_length + 1));
        strcpy(str, str_offset);
        char *temp = str;
        while (*temp) {
            *temp = toupper(*temp);
            temp++;
        }
        ASSERT_EQ(strcasecmp(str, str_offset), 0);
        free(str);
        str = (char *)malloc(sizeof(char) * (alloc_length + 1));
        fill_randomstring(str, alloc_length);
        temp = str;
        while (*temp) {
            *temp = toupper(*temp);
            temp++;
        }
        ASSERT_NE(strcasecmp(str, str_offset), 0);
        free(str);
        free(buff);
    }
}

TEST(STRING, strncasecmp) {
    const char str1[] = "ABCDEFrtyhjfGHACBkleqdgbmcKLQSWTXYZ";
    char *str2;
    size_t str_length = strlen(str1);
    str2 = (char *)malloc(sizeof(char) * str_length);
    strcpy(str2, str1);
    char *str3 = str2;
    while (*str3) {
        *str3 = toupper(*str3);
        str3++;
    }
    ASSERT_EQ(strncasecmp(str1, str2, str_length / 2), 0);
    free(str2);
}

TEST(STRING, mallocstrncasecmp) {
    srand(SRAND_SEED);
    char *buff, *str_offset, *str;
    size_t max_length = 128;
    for (size_t alloc_length = 5; alloc_length < max_length; alloc_length++) {
        buff = (char *)malloc(sizeof(char) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (size_t length_expected = 3; length_expected < alloc_length;
                 length_expected++) {
                fill_string(str_offset, length_expected);
                str = (char *)malloc(sizeof(char) * (length_expected + 1));
                strcpy(str, str_offset);
                char *temp = str;
                while (*temp) {
                    *temp = toupper(*temp);
                    temp++;
                }

                // validating all 3 cases when length is less/equal/larger than
                // the actual string length
                ASSERT_EQ(strncasecmp(str, str_offset, length_expected / 2), 0);
                ASSERT_EQ(strncasecmp(str, str_offset, length_expected), 0);
                ASSERT_EQ(strncasecmp(str, str_offset, (length_expected + 10)),
                          0);
                free(str);
                str = (char *)malloc(sizeof(char) * (length_expected + 1));
                fill_randomstring(str, length_expected);
                temp = str;
                while (*temp) {
                    *temp = toupper(*temp);
                    temp++;
                }
                ASSERT_NE(strncasecmp(str, str_offset, length_expected / 2), 0);
                free(str);
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocstrncasecmprandomsize) {
    srand(SRAND_SEED);
    char *buff, *str_offset, *str;
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    for (size_t i = 1; i <= 100; i++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (char *)malloc(sizeof(char) * (alloc_length + offset + 1));
        str_offset = buff + offset;
        fill_string(str_offset, alloc_length);
        str = (char *)malloc(sizeof(char) * (alloc_length + 1));
        strcpy(str, str_offset);
        char *temp = str;
        while (*temp) {
            *temp = toupper(*temp);
            temp++;
        }
        // validating all 3 cases when length is less/equal/larger than the
        // actual string length
        ASSERT_EQ(strncasecmp(str, str_offset, alloc_length / 2), 0);
        ASSERT_EQ(strncasecmp(str, str_offset, alloc_length), 0);
        ASSERT_EQ(strncasecmp(str, str_offset, (alloc_length + 10)), 0);
        free(str);
        str = (char *)malloc(sizeof(char) * (alloc_length + 1));
        fill_randomstring(str, alloc_length);
        temp = str;
        while (*temp) {
            *temp = toupper(*temp);
            temp++;
        }
        ASSERT_NE(strncasecmp(str, str_offset, alloc_length / 2), 0);
        free(str);
        free(buff);
    }
}

TEST(STRING, strcmp) {
    char *str1, *str2;
    size_t str_length = 100;
    str1 = (char *)malloc(sizeof(char) * str_length);
    fill_string(str1, str_length);
    str2 = (char *)malloc(sizeof(char) * strlen(str1));
    strcpy(str2, str1);
    ASSERT_FALSE(strcmp(str1, str2));
    free(str1);
    free(str2);
}

TEST(STRING, mallocstrcmp) {
    srand(SRAND_SEED);
    char *buff, *str_offset, *str;
    size_t max_length = 128;
    for (size_t alloc_length = 2; alloc_length < max_length; alloc_length++) {
        buff = (char *)malloc(sizeof(char) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (size_t length_expected = 1; length_expected < alloc_length;
                 length_expected++) {
                fill_string(str_offset, length_expected);
                str = (char *)malloc(sizeof(char) * (length_expected + 1));
                strcpy(str, str_offset);
                ASSERT_EQ(strcmp(str, str_offset), 0);
                free(str);
                str = (char *)malloc(sizeof(char) * (length_expected + 1));
                fill_randomstring(str, length_expected);
                ASSERT_NE(strcmp(str, str_offset), 0);
                free(str);
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocstrcmprandomsize) {
    srand(SRAND_SEED);
    char *buff, *str_offset, *str;
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    for (size_t i = 1; i <= 100; i++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (char *)malloc(sizeof(char) * (alloc_length + offset + 1));
        str_offset = buff + offset;
        fill_string(str_offset, alloc_length);
        str = (char *)malloc(sizeof(char) * (alloc_length + 1));
        strcpy(str, str_offset);
        ASSERT_EQ(strcmp(str, str_offset), 0);
        free(str);
        str = (char *)malloc(sizeof(char) * (alloc_length + 1));
        fill_randomstring(str, alloc_length);
        ASSERT_NE(strcmp(str, str_offset), 0);
        free(str);
        free(buff);
    }
}

TEST(STRING, mallocstrncmp) {
    char *buff, *str_offset, *str;
    size_t max_length = 128;
    for (size_t alloc_length = 5; alloc_length < max_length; alloc_length++) {
        buff = (char *)malloc(sizeof(char) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (size_t length_expected = 3; length_expected < alloc_length;
                 length_expected++) {
                fill_string(str_offset, length_expected);
                str = (char *)malloc(sizeof(char) * (length_expected + 1));
                strcpy(str, str_offset);
                // validating all 3 cases when length is less/equal/larger than
                // the actual string length
                ASSERT_EQ(strncmp(str, str_offset, length_expected / 2), 0);
                ASSERT_EQ(strncmp(str, str_offset, length_expected), 0);
                ASSERT_EQ(strncmp(str, str_offset, length_expected + 10), 0);
                free(str);
                str = (char *)malloc(sizeof(char) * (length_expected + 1));
                fill_randomstring(str, length_expected);
                ASSERT_NE(strncmp(str, str_offset, length_expected / 2), 0);
                free(str);
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocstrncmprandomsize) {
    char *buff, *str_offset, *str;
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    for (size_t i = 1; i <= 100; i++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (char *)malloc(sizeof(char) * (alloc_length + offset + 1));
        str_offset = buff + offset;
        fill_string(str_offset, alloc_length);
        str = (char *)malloc(sizeof(char) * (alloc_length + 1));
        strcpy(str, str_offset);
        // validating all 3 cases when length is less/equal/larger than the
        // actual string length
        ASSERT_EQ(strncmp(str, str_offset, alloc_length / 2), 0);
        ASSERT_EQ(strncmp(str, str_offset, alloc_length), 0);
        ASSERT_EQ(strncmp(str, str_offset, alloc_length + 10), 0);
        free(str);
        str = (char *)malloc(sizeof(char) * (alloc_length + 1));
        fill_randomstring(str, alloc_length);
        ASSERT_NE(strncmp(str, str_offset, alloc_length / 2), 0);
        free(str);
        free(buff);
    }
}

TEST(STRING, strstr) {
    char *str1;
    const char str2[] = "abcdefgh";
    size_t str_length = 100;
    str1 = (char *)malloc(sizeof(char) * str_length);
    fill_string(str1, str_length);
    ASSERT_NE(strstr(str1, str2), nullptr);
    free(str1);
}

TEST(STRING, mallocstrstr) {
    srand(SRAND_SEED);
    char *buff, *str_offset, *str;
    size_t max_length = 128;
    for (size_t alloc_length = 5; alloc_length <= max_length; alloc_length++) {
        buff = (char *)malloc(sizeof(char) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;

            fill_string(str_offset, alloc_length);
            for (size_t length_expected = 3; length_expected < alloc_length;
                 length_expected++) {
                str = (char *)malloc(sizeof(char) * (length_expected + 1));
                fill_string(str, length_expected);
                ASSERT_NE(strstr(str_offset, str), nullptr);
                free(str);
                str = (char *)malloc(sizeof(char) * (length_expected + 1));
                fill_randomstring(str, length_expected);
                ASSERT_EQ(strstr(str_offset, str), nullptr);
                free(str);
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocstrstrrandomsize) {
    srand(SRAND_SEED);
    char *buff, *str_offset, *str;
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    for (size_t i = 1; i <= 100; i++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (char *)malloc(sizeof(char) * (alloc_length + offset + 1));
        str_offset = buff + offset;
        fill_string(str_offset, alloc_length);
        str = (char *)malloc(sizeof(char) * (alloc_length + 1));
        fill_string(str, alloc_length);
        ASSERT_NE(strstr(str_offset, str), nullptr);
        free(str);
        str = (char *)malloc(sizeof(char) * (alloc_length + 1));
        fill_randomstring(str, alloc_length);
        ASSERT_EQ(strstr(str_offset, str), nullptr);
        free(str);
        free(buff);
    }
}

TEST(STRING, strcasestr) {
    char *str1;
    const char str2[] = "AbCdEfGHijk";
    size_t str_length = 100;
    str1 = (char *)malloc(sizeof(char) * str_length);
    fill_string(str1, str_length);
    ASSERT_TRUE(strcasestr(str1, str2));
    free(str1);
}

TEST(STRING, mallocstrcasestr) {
    srand(SRAND_SEED);
    char *buff, *str_offset, *str;
    size_t max_length = 128;
    for (size_t alloc_length = 5; alloc_length < max_length; alloc_length++) {
        buff = (char *)malloc(sizeof(char) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            fill_string(str_offset, alloc_length);
            for (size_t length_expected = 3; length_expected < alloc_length;
                 length_expected++) {
                str = (char *)malloc(sizeof(char) * (length_expected + 1));
                fill_string(str, length_expected);
                char *temp = str;
                while (*temp) {
                    *temp = toupper(*temp);
                    temp++;
                }
                ASSERT_NE(strcasestr(str_offset, str), nullptr);
                free(str);
                str = (char *)malloc(sizeof(char) * (length_expected + 1));
                fill_randomstring(str, length_expected);
                temp = str;
                while (*temp) {
                    *temp = toupper(*temp);
                    temp++;
                }
                ASSERT_EQ(strcasestr(str_offset, str), nullptr);
                free(str);
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocstrcasestrrandsize) {
    srand(SRAND_SEED);
    char *buff, *str_offset, *str;
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    for (size_t i = 1; i <= 100; i++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (char *)malloc(sizeof(char) * (alloc_length + offset + 1));
        str_offset = buff + offset;
        fill_string(str_offset, alloc_length);
        str = (char *)malloc(sizeof(char) * (alloc_length + 1));
        fill_string(str, alloc_length);
        char *temp = str;
        while (*temp) {
            *temp = toupper(*temp);
            temp++;
        }
        ASSERT_NE(strcasestr(str_offset, str), nullptr);
        free(str);
        str = (char *)malloc(sizeof(char) * (alloc_length + 1));
        fill_randomstring(str, alloc_length);
        temp = str;
        while (*temp) {
            *temp = toupper(*temp);
            temp++;
        }
        ASSERT_EQ(strcasestr(str_offset, str), nullptr);
        free(str);
        free(buff);
    }
}

TEST(STRING, mallocstrtok) {
    srand(SRAND_SEED);
    char *buff, *str_offset, *str, *tmpstr;
    char delim[] = "}";
    size_t str_length = 93;
    size_t max_length = 128;
    str = (char *)malloc(sizeof(char) * max_length);
    fill_string(str, str_length - 1);
    for (size_t alloc_length = 5; alloc_length < max_length; alloc_length++) {
        buff = (char *)malloc(sizeof(char) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (size_t length_expected = 3; length_expected < alloc_length;
                 length_expected++) {
                char *token;
                fill_string(str_offset, length_expected);
                tmpstr = (char *)malloc(sizeof(char) * (length_expected + 1));
                fill_string(tmpstr, length_expected);
                token = strtok(str_offset, delim);
                while (token) {
                    if (length_expected <= (str_length - 1)) {
                        ASSERT_STREQ(tmpstr, str_offset);
                    } else {
                        ASSERT_STREQ(str, str_offset);
                    }
                    token = strtok(NULL, delim);
                }
                free(tmpstr);
            }
        }
        free(buff);
    }
    free(str);
}

TEST(STRING, mallocstrtokrandomsize) {
    srand(SRAND_SEED);
    char *buff, *str_offset, *str, *tmpstr;
    char delim[] = "}";
    size_t str_length = 93;
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    str = (char *)malloc(sizeof(char) * max_length);
    fill_string(str, str_length - 1);
    for (size_t i = 1; i <= 100; i++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (char *)malloc(sizeof(char) * (alloc_length + offset + 1));
        str_offset = buff + offset;
        char *token;
        fill_string(str_offset, alloc_length);
        tmpstr = (char *)malloc(sizeof(char) * (alloc_length + 1));
        fill_string(tmpstr, alloc_length);
        token = strtok(str_offset, delim);
        while (token) {
            if (alloc_length <= (str_length - 1)) {
                ASSERT_STREQ(tmpstr, str_offset);
            } else {
                ASSERT_STREQ(str, str_offset);
            }
            token = strtok(NULL, delim);
        }
        free(tmpstr);
        free(buff);
    }
    free(str);
}

TEST(STRING, mallocstpcpyrandomsize) {
    srand(SRAND_SEED);
    char *buff, *str_offset, *str;
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    for (size_t i = 1; i <= 100; i++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (char *)malloc(sizeof(char) * (alloc_length + offset + 1));
        str_offset = buff + offset;
        fill_string(str_offset, alloc_length);
        str = (char *)malloc(sizeof(char) * (alloc_length + 1));
        char *ret = stpcpy(str, str_offset);
        char value = *ret;
        ASSERT_STREQ(str, str_offset);
        ASSERT_EQ(value, '\0');
        free(str);
        free(buff);
    }
}

TEST(STRING, mallocstpncpy) {
    srand(SRAND_SEED);
    char *buff, *str_offset, *str;
    size_t max_length = 128;
    for (size_t alloc_length = 5; alloc_length < max_length; alloc_length++) {
        buff = (char *)malloc(sizeof(char) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (size_t length_expected = 3; length_expected < alloc_length;
                 length_expected++) {
                fill_string(str_offset, length_expected);
                str = (char *)malloc(sizeof(char) * (length_expected + 1));
                stpncpy(str, str_offset, length_expected - 2);
                str[length_expected - 2] = '\0';
                ASSERT_STRNE(str, str_offset);
                free(str);
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocstpncpyrandomsize) {
    srand(SRAND_SEED);
    char *buff, *str_offset, *str;
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    for (size_t i = 1; i <= 100; i++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (char *)malloc(sizeof(char) * (alloc_length + offset + 1));
        str_offset = buff + offset;
        fill_string(str_offset, alloc_length);
        str = (char *)malloc(sizeof(char) * (alloc_length + 1));
        stpncpy(str, str_offset, alloc_length - 2);
        str[alloc_length - 2] = '\0';
        ASSERT_STRNE(str, str_offset);
        free(str);
        free(buff);
    }
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
