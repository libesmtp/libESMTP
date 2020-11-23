#include <check.h>
#include <stdlib.h>
#include <string.h>
#include "concatenate.h"
#include "concatenate.c"

START_TEST (concatenate_base)
{
  struct catbuf catbuf;

  /* Initialise buffer and check lengths */
  cat_init (&catbuf, 32);
  ck_assert_int_eq (catbuf.allocated, 32);
  ck_assert_int_eq (catbuf.string_length, 0);

  /* add some data */
  concatenate (&catbuf, "test", -1);
  ck_assert_int_eq (catbuf.allocated, 32);
  ck_assert_int_eq (catbuf.string_length, 4);
  ck_assert_int_eq (memcmp (catbuf.buffer, "test", 4), 0);

  /* add some more data */
  concatenate (&catbuf, "abcdefghijklmnopqrstuvwxyz", -1);
  ck_assert_int_eq (catbuf.allocated, 32);
  ck_assert_int_eq (catbuf.string_length, 30);
  ck_assert_int_eq (memcmp (catbuf.buffer, "testabcdefghijklmnopqrstuvwxyz", 30), 0);

  /* overflow original allocation */
  concatenate (&catbuf, "1234567890", -1);
  ck_assert_int_ge (catbuf.allocated, 40);
  ck_assert_int_eq (catbuf.string_length, 40);
  ck_assert_int_eq (memcmp (catbuf.buffer, "testabcdefghijklmnopqrstuvwxyz1234567890", 40), 0);

  /* check only requested length copied */
  concatenate (&catbuf, "ABCDEFGH", 2);
  ck_assert_int_ge (catbuf.allocated, 42);
  ck_assert_int_eq (catbuf.string_length, 42);
  ck_assert_int_eq (memcmp (catbuf.buffer, "testabcdefghijklmnopqrstuvwxyz1234567890AB", 42), 0);

  /* forcibly resize buffer */
  cat_alloc (&catbuf, 64);
  ck_assert_int_eq (catbuf.allocated, 64);
  ck_assert_int_eq (catbuf.string_length, 42);
  ck_assert_int_eq (memcmp (catbuf.buffer, "testabcdefghijklmnopqrstuvwxyz1234567890AB", 42), 0);

  /* shrink buffer */
  cat_shrink (&catbuf, NULL);
  ck_assert_int_eq (catbuf.allocated, 42);
  ck_assert_int_eq (catbuf.string_length, 42);
  ck_assert_int_eq (memcmp (catbuf.buffer, "testabcdefghijklmnopqrstuvwxyz1234567890AB", 42), 0);

  cat_free (&catbuf);
}
END_TEST

START_TEST (concatenate_more)
{
  struct catbuf catbuf;

  cat_init (&catbuf, 4);
  vconcatenate (&catbuf, "abcdef", "gh", "ijklmno", "p", "", "qrstuvwxyz", NULL);
  ck_assert_int_ge (catbuf.allocated, 26);
  ck_assert_int_eq (catbuf.string_length, 26);
  ck_assert_int_eq (memcmp (catbuf.buffer, "abcdefghijklmnopqrstuvwxyz", 26), 0);

  cat_printf (&catbuf, " %d %s\n", 10, "xyzzy");
  ck_assert_int_ge (catbuf.allocated, 36);
  ck_assert_int_eq (catbuf.string_length, 36);
  ck_assert_int_eq (memcmp (catbuf.buffer, "abcdefghijklmnopqrstuvwxyz 10 xyzzy\n", 36), 0);

  cat_free (&catbuf);
}
END_TEST

static Suite *
concatenate_suite (void)
{
  Suite *s;
  TCase *tc_core;

  s = suite_create ("Concatenate");

  tc_core = tcase_create ("Basic Operation");
  tcase_add_test (tc_core, concatenate_base);
  suite_add_tcase (s, tc_core);

  tc_core = tcase_create ("Other Functions");
  tcase_add_test (tc_core, concatenate_more);
  suite_add_tcase (s, tc_core);

  return s;
}


int
main (void)
{
  int number_failed;
  Suite *s;
  SRunner *sr;

  s = concatenate_suite ();

  sr = srunner_create (s);
  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);

  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
