#include <check.h>
#include <stdlib.h>
#include <strings.h>
#include "missing.c"

START_TEST (test_strdup)
{
  char *duped;

  duped = strdup ("testing");
  ck_assert_str_eq (duped, "testing");
  free (duped);
}
END_TEST

START_TEST (test_strcasecmp)
{
  const char str1[] = "THIS IS A TEST";
  const char str2[] = "THIS IS A TEST";
  const char str3[] = "this is a test";
  const char str4[] = "This Is a Test";
  const char str5[] = "This Is b Test";
  const char str6[] = "This Is B Test";
  const char str7[] = "This Is";

  ck_assert_int_eq (strcasecmp (str1, str1), 0);
  ck_assert_int_eq (strcasecmp (str1, str2), 0);
  ck_assert_int_eq (strcasecmp (str1, str3), 0);
  ck_assert_int_eq (strcasecmp (str1, str4), 0);
  ck_assert_int_eq (strcasecmp (str1, str5), -1);
  ck_assert_int_eq (strcasecmp (str5, str1), 1);
  ck_assert_int_eq (strcasecmp (str1, str6), -1);
  ck_assert_int_eq (strcasecmp (str6, str1), 1);
  ck_assert_int_gt (strcasecmp (str1, str7), 0);
  ck_assert_int_lt (strcasecmp (str7, str1), 0);
}
END_TEST

START_TEST (test_strncasecmp)
{
  const char str1[] = "THIS IS A TEST";
  const char str2[] = "THIS IS A TEST";
  const char str3[] = "this is a test";
  const char str4[] = "This Is a Test";
  const char str5[] = "This Is b Test";
  const char str6[] = "This Is B Test";
  const char str7[] = "This Is";

  ck_assert_int_eq (strncasecmp (str1, str1, 14), 0);
  ck_assert_int_eq (strncasecmp (str1, str2, 14), 0);
  ck_assert_int_eq (strncasecmp (str1, str3, 14), 0);
  ck_assert_int_eq (strncasecmp (str1, str4, 14), 0);
  ck_assert_int_eq (strncasecmp (str1, str5, 14), -1);
  ck_assert_int_eq (strncasecmp (str5, str1, 14), 1);
  ck_assert_int_eq (strncasecmp (str1, str6, 14), -1);
  ck_assert_int_eq (strncasecmp (str6, str1, 14), 1);

  ck_assert_int_gt (strncasecmp (str1, str7, 12), 0);
  ck_assert_int_lt (strncasecmp (str7, str1, 12), 0);

  ck_assert_int_eq (strncasecmp (str1, str2, 12), 0);
  ck_assert_int_eq (strncasecmp (str1, str2, 16), 0);
}
END_TEST

START_TEST (test_memrchr)
{
  const char str1[] = "THIS IS A TEST";

  ck_assert_ptr_eq (memrchr (str1, ' ', sizeof str1), &str1[9]);
  ck_assert_ptr_eq (memrchr (str1, 'S', sizeof str1), &str1[12]);
  ck_assert_ptr_eq (memrchr (str1, 'X', sizeof str1), NULL);
  ck_assert_ptr_eq (memrchr (str1, '\0', sizeof str1), &str1[14]);

  ck_assert_ptr_eq (memrchr (str1, ' ', 10), &str1[9]);
  ck_assert_ptr_eq (memrchr (str1, 'S', 10), &str1[6]);
  ck_assert_ptr_eq (memrchr (str1, 'X', 10), NULL);
  ck_assert_ptr_eq (memrchr (str1, '\0', 10), NULL);
}
END_TEST

START_TEST (test_strlcpy)
{
  const char str1[] = "THIS IS A TEST";
  char dest[20];

  /* basic use - dest is sufficiently large */
  ck_assert_int_eq (strlcpy (dest, str1, sizeof dest), 14);
  ck_assert_int_eq (dest[14], '\0');
  ck_assert_str_eq (dest, str1);

  /* check truncation and return value */
  dest[10] = 'X';
  ck_assert_int_eq (strlcpy (dest, str1, 10), 14);
  ck_assert_int_eq (dest[9], '\0');
  ck_assert_int_eq (dest[10], 'X');
  ck_assert_str_eq (dest, "THIS IS A");
}
END_TEST

static Suite *
missing_suite (void)
{
  Suite *s;
  TCase *tc_core;

  s = suite_create ("Missing");

  tc_core = tcase_create ("Strdup");
  tcase_add_test (tc_core, test_strdup);
  suite_add_tcase (s, tc_core);

  tc_core = tcase_create ("Strcasecmp");
  tcase_add_test (tc_core, test_strcasecmp);
  suite_add_tcase (s, tc_core);

  tc_core = tcase_create ("Strncasecmp");
  tcase_add_test (tc_core, test_strncasecmp);
  suite_add_tcase (s, tc_core);

  tc_core = tcase_create ("Memrchr");
  tcase_add_test (tc_core, test_memrchr);
  suite_add_tcase (s, tc_core);

  tc_core = tcase_create ("Strlcpy");
  tcase_add_test (tc_core, test_strlcpy);
  suite_add_tcase (s, tc_core);

  return s;
}


int
main (void)
{
  int number_failed;
  Suite *s;
  SRunner *sr;

  s = missing_suite ();

  sr = srunner_create (s);
  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);

  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
