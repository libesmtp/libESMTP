#include <check.h>
#include <stdlib.h>
#include "tlsutils.h"
#include "tlsutils.c"

START_TEST (test_match_domain)
{
  /* basic comparisons */
  ck_assert_int_eq (match_domain ("MAIL.EXAMPLE.COM", "MAIL.EXAMPLE.COM"), 1);
  ck_assert_int_eq (match_domain ("MAIL.EXAMPLE.COM", "mail.example.com"), 1);
  ck_assert_int_eq (match_domain ("mail.example.com", "MAIL.EXAMPLE.COM"), 1);

  /* mismatched components */
  ck_assert_int_eq (match_domain ("MAIL.EXAMPLE.COM", "email.example.com"), 0);
  ck_assert_int_eq (match_domain ("EMAIL.EXAMPLE.COM", "mail.example.com"), 0);
  ck_assert_int_eq (match_domain ("MAIL.EXAMPLE.COM", "mail.example.org"), 0);
  ck_assert_int_eq (match_domain ("MAIL.EXAMPLE.COM", "mail.example.org"), 0);

  /* too many components */
  ck_assert_int_eq (match_domain ("MAIL.EXAMPLE.COM", "example.com"), 0);

  /* too few components */
  ck_assert_int_eq (match_domain ("EXAMPLE.COM", "mail.example.com"), 0);

#if 0
  /* starting with a dot */
  ck_assert_int_eq (match_domain ("xyz.com", ".xyz.com"), 0);
  ck_assert_int_eq (match_domain ("abc.xyz.com", ".xyz.com"), 1);
  ck_assert_int_eq (match_domain ("abc.def.xyz.com", ".xyz.com"), 1);
#endif

  /* wildcards */
  ck_assert_int_eq (match_domain ("MAIL.EXAMPLE.COM", "*.example.com"), 1);
  ck_assert_int_eq (match_domain ("MAIL.EXAMPLE.COM", "mail.*.com"), 1);//XXX
  ck_assert_int_eq (match_domain ("MAIL.EXAMPLE.COM", "*.*.*"), 1);//XXX

  /* invalid wildcards */
  ck_assert_int_eq (match_domain ("MAIL.EXAMPLE.COM", "*.*ple.com"), 0);
  ck_assert_int_eq (match_domain ("MAIL.EXAMPLE.COM", "m*.example.com"), 0);

  /* garbage */
  ck_assert_int_eq (match_domain ("MAIL!.EXAMPLE.COM", "mail!.example.com"), 0);
  ck_assert_int_eq (match_domain ("MAIL_.EXAMPLE.COM", "mail_.example.com"), 0);
  ck_assert_int_eq (match_domain ("MAIL-.EXAMPLE.COM", "mail-.example.com"), 1); //XXX
}
END_TEST

Suite *
tlsutils_suite (void)
{
  Suite *s;
  TCase *tc_core;

  s = suite_create ("TLS Utils");

  tc_core = tcase_create ("Match Domain");
  tcase_add_test (tc_core, test_match_domain);
  suite_add_tcase (s, tc_core);

  return s;
}


int
main (void)
{
  int number_failed;
  Suite *s;
  SRunner *sr;

  s = tlsutils_suite ();

  sr = srunner_create (s);
  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);

  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
