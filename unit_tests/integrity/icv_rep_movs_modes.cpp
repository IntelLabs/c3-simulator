// model: cc-integrity c3-integrity
// simics_args: enable_integrity=1
// should_fail: yes

#include <gtest/gtest.h>
#include "malloc/cc_globals.h"
#include "unit_tests/common.h"

#define INV_OFFSET 16
#define MAGIC_VAL_INTRA 0xdeadbeefd0d0caca

static const char *str = "Hello World AAAAAAAAAAAAAAAAAA!";
static const char *str2 = "Hello World ABCDEFGHIJKLMNOPQ!";

// This is a negative test, and should fail.

TEST(Integrity, test_icv_rep_movs_modes) {
    if (!is_model("native")) {
        cc_set_icv_enabled(true);
    }
    char *src = (char *)malloc(128);
    char *dst = (char *)malloc(128);
    // Copy over example string to CA src
    strncpy(src, str, 128);

    printf("The string at src (%p): '%s'\n", src, src);

    int n = strlen(str) + 1;
    printf("Copying string at src to dst with ICVs preserved\n");
    printf("DS REP MOVS src=%p dst=%p, n=%d\n", src, dst, n);
    cc_icv_memcpy(dst, src, n);
    printf("The string at src (%p): '%s'\n", src, src);
    printf("The string at dst (%p): '%s'\n", dst, dst);
    EXPECT_STREQ(src, dst);

    printf("Now invalidating src (%p) + %d\n", src, INV_OFFSET);
    *(uint64_t *)(src + INV_OFFSET) = MAGIC_VAL_INTRA;
    *(uint64_t *)(src + INV_OFFSET - 1) =
            '\0';  // Null terminate string so EXPECT_STREQ works
    cc_isa_invicv(src + INV_OFFSET);
    printf("The string at src (after invalidation): '%s'\n", src);

    printf("Copying string at src to dst with ICVs preserved (should succeed, "
           "only src has invalidated)\n");
    cc_icv_memcpy(dst, src, n);

    printf("Again copying string at src to dst with ICVs preserved (should not "
           "fail for PERMISSIVE)\n");
    printf("Offset %d in dst should be invalid -> no fault for permissive\n",
           INV_OFFSET);
    cc_icv_memcpy_permissive(dst, src, n);
    EXPECT_STREQ(src, dst);

    printf("Try copying src to new misaligned location with ICVs preserved "
           "(should not fail)\n");
    char *extra = (char *)malloc(128);
    cc_icv_memcpy(extra + 4, src, n);
    EXPECT_STREQ(extra + 4, src);

    printf("Try copying src + 4 to new aligned buffer with ICVs preserved "
           "(should not fail)\n");
    char *extra2 = (char *)malloc(128);
    cc_icv_memcpy(extra2, src + 4, n);
    EXPECT_STREQ(extra2, src + 4);

    printf("Try copying src + 4 to new misaligned buffer with ICVs preserved "
           "(should not fail)\n");
    char *extra3 = (char *)malloc(128);
    cc_icv_memcpy(extra3 + 4, src + 4, n);
    EXPECT_STREQ(extra3 + 4, src + 4);

    char *src_move = (char *)malloc(128);
    strncpy(src_move, str2, 128);
    size_t n_move = strlen(src_move) + 1;
    printf("Now memmove string (len=%lu) at new src to new dst with ICVs "
           "preserved (should not fail)\n",
           n_move);
    EXPECT_STREQ(src_move, str2);
    char *dst_move = (char *)malloc(128);
    cc_icv_memmove(dst_move, src_move, n_move);
    EXPECT_STREQ(dst_move, src_move);

    printf("Try memmove src to new misaligned location with ICVs preserved "
           "(should not fail)\n");
    char *extra_move = (char *)malloc(128);
    cc_icv_memmove(extra_move + 4, src_move, n_move);
    EXPECT_STREQ(extra_move + 4, src_move);
    printf("extra_move + 4: %s\n", extra_move + 4);

    printf("Try memmove src + 4 to new aligned buffer with ICVs preserved "
           "(should not fail)\n");
    char *extra_move2 = (char *)malloc(128);
    cc_icv_memmove(extra_move2, src_move + 4, n_move - 4);
    EXPECT_STREQ(extra_move2, src_move + 4);

    printf("Try memmove src + 4 to new misaligned buffer with ICVs preserved "
           "(should not fail)\n");
    char *extra_move3 = (char *)malloc(128);
    cc_icv_memmove(extra_move3 + 4, src_move + 4, n_move - 4);
    EXPECT_STREQ(extra_move3 + 4, src_move + 4);

    printf("Try memmove src to src + 4 (should not fail)\n");
    char *extra_move_overlap = (char *)malloc(128);
    strncpy(extra_move_overlap, src_move, 128);
    cc_icv_memmove(extra_move_overlap + 4, extra_move_overlap, n_move);
    EXPECT_STREQ(extra_move_overlap + 4, src_move);

    printf("Yet Again copying string at src to dst with ICVs preserved (should "
           "now fail for STRICT)\n");
    printf("Offset %d in dst should be invalid -> fault for STRICT\n",
           INV_OFFSET);

    // Avoid EXPECT_DEATH since ICVs seem to interact badly with it
    // EXPECT_DEATH(cc_icv_memcpy(dst, src, n), ".*");
    cc_icv_memcpy(dst, src, n);

    printf("Testing memmove propagation (should fail for STRICT)\n");
    char *extra_move4 = (char *)malloc(128);
    *(uint64_t *)(extra_move3 + INV_OFFSET) = MAGIC_VAL_INTRA;
    cc_isa_invicv(extra_move3 + INV_OFFSET);
    cc_icv_memmove(extra_move4, extra_move3, n_move);
    printf("SHOULD FAIL NOW\n");
    *(extra_move4 + INV_OFFSET) = 'X';

    printf("ERROR (should not reach here)\n");

    free(src);
    free(dst);
    free(extra);
    free(extra2);
    free(extra3);
    free(src_move);
    free(dst_move);
    free(extra_move);
    free(extra_move2);
    free(extra_move3);
    free(extra_move4);

    if (!is_model("native")) {
        cc_set_icv_enabled(false);
    }
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
