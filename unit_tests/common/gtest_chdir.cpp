// model: *
#include <unistd.h>
#include <gtest/gtest.h>

using namespace std;  // NOLINT

// #define dbgprint(...) fprintf(stderr, __VA_ARGS__)
#ifndef dbgprint
#define dbgprint(...)
#endif

#define _PATH "/tmp"

const char *g_path = _PATH;

class ChdirTest : public ::testing::Test {
 protected:
    const char *m_path = _PATH;
    char *h_path = nullptr;

 public:
    ChdirTest() {
        h_path = (char *)malloc(sizeof(_PATH));
        strncpy(h_path, _PATH, sizeof(_PATH));
    }

    ~ChdirTest() { free(h_path); }
};

__attribute__((noinline)) int indirect_chdir(const char *path) {
    return chdir(path);
}

TEST_F(ChdirTest, basic_chdir_stack) {
    const char *l_path = _PATH;
    ASSERT_TRUE(0 == chdir(l_path));
}

TEST_F(ChdirTest, basic_chdir_global) { ASSERT_TRUE(0 == chdir(g_path)); }

TEST_F(ChdirTest, basic_chdir_heap) { ASSERT_TRUE(0 == chdir(h_path)); }

TEST_F(ChdirTest, basic_chdir_objd) { ASSERT_TRUE(0 == chdir(m_path)); }

TEST_F(ChdirTest, indirect_chdir_stack) {
    const char *l_path = _PATH;
    ASSERT_TRUE(0 == indirect_chdir(l_path));
}

TEST_F(ChdirTest, indirect_chdir_global) {
    ASSERT_TRUE(0 == indirect_chdir(g_path));
}

TEST_F(ChdirTest, indirect_chdir_heap) {
    ASSERT_TRUE(0 == indirect_chdir(h_path));
}

TEST_F(ChdirTest, indirect_chdir_objd) {
    ASSERT_TRUE(0 == indirect_chdir(m_path));
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
