#include <check.h>
#include <stdlib.h>
#include <string.h>
#include "base64.h"
#include "base64.c"

START_TEST (base64_encode)
{
  char buffer[256];
  char data[] =
    {
      0x46, 0xb2, 0x7d, 0xac, 0x2f,
      0xad, 0xcc, 0x46, 0xdb, 0xac,
      0x01, 0xce, 0xa9, 0xda, 0xde,
      0x4b, 0x74, 0xc1, 0xaa,
    };
  char b64data[] = "RrJ9rC+tzEbbrAHOqdreS3TBqg==";

  strcpy (&buffer[29], "MaGiC");

  /* encode, check output length */
  ck_assert_int_eq (b64_encode (buffer, sizeof buffer, data, sizeof data), 28);
  /* verify result */
  ck_assert_str_eq  (buffer, b64data);
  /* verify \0 termination */
  ck_assert_int_eq (buffer[28], '\0');
  /* check for buffer overrun */
  ck_assert_str_eq (&buffer[29], "MaGiC");
  /* check input buffer not NULL */
  ck_assert_int_eq (b64_encode (buffer, sizeof buffer, NULL, 0), 0);
  /* check output buffer has enough space for \0 */
  ck_assert_int_eq (b64_encode (buffer, 28, data, sizeof data), -1);
  /* check output buffer has enough space for result */
  ck_assert_int_eq (b64_encode (buffer, 16, data, sizeof data), -1);
}
END_TEST

START_TEST (base64_decode)
{
  char buffer[256];
  char data[] =
    {
      0x46, 0xb2, 0x7d, 0xac, 0x2f,
      0xad, 0xcc, 0x46, 0xdb, 0xac,
      0x01, 0xce, 0xa9, 0xda, 0xde,
      0x4b, 0x74, 0xc1, 0xaa,
    };
  char b64data[] = "RrJ9rC+tzEbbrAHOqdreS3TBqg==";

  strcpy (&buffer[19], "MaGiC");

  /* decode, check output length */
  ck_assert_int_eq (b64_decode (buffer, sizeof buffer, b64data, sizeof b64data - 1), 19);
  /* verify result */
  ck_assert_int_eq  (memcmp (buffer, data, sizeof data), 0);
  /* check for buffer overrun */
  ck_assert_str_eq (&buffer[19], "MaGiC");
  /* check input buffer not NULL */
  ck_assert_int_eq (b64_decode (buffer, sizeof buffer, NULL, 0), 0);
  /* check output buffer has enough space for result */
  ck_assert_int_eq (b64_decode (buffer, 18, b64data, sizeof b64data), -1);
  /* check against invalid input */
  ck_assert_int_eq (b64_decode (buffer, sizeof buffer, data, sizeof data), -1);
}
END_TEST

Suite *
base64_suite (void)
{
  Suite *s;
  TCase *tc_core;

  s = suite_create ("Base64");

  tc_core = tcase_create ("Base64 Encode");
  tcase_add_test (tc_core, base64_encode);
  suite_add_tcase (s, tc_core);

  tc_core = tcase_create ("Base64 Decode");
  tcase_add_test (tc_core, base64_decode);
  suite_add_tcase (s, tc_core);

  return s;
}


int
main (void)
{
  int number_failed;
  Suite *s;
  SRunner *sr;

  s = base64_suite ();

  sr = srunner_create (s);
  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);

  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
