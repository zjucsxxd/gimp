/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 2011 Róman Joost <romanofski@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>

#include <glib.h>
#include <gtk/gtk.h>

#include "xmp-parse.h"
#include "xmp-encode.h"
#include "xmp-model.h"


#define ADD_TEST(function) \
  g_test_add ("/metadata-xmp-model/" #function, \
              GimpTestFixture, \
              NULL, \
              gimp_test_xmp_model_setup, \
              function, \
              gimp_test_xmp_model_teardown);


typedef struct
{
  XMPModel *xmp_model;
} GimpTestFixture;


typedef struct
{
  const gchar *schema_name;
  const gchar *name;
  int          pos;
  const gchar *expected_value;
  const gchar *expected_values[10];
} TestDataEntry;

static TestDataEntry propertiestotest[] =
{
   { XMP_PREFIX_DUBLIN_CORE,  "title",          1, NULL, { NULL } },
   { XMP_PREFIX_DUBLIN_CORE,  "creator",        0, NULL, { NULL } },
   { XMP_PREFIX_DUBLIN_CORE,  "description",    1, NULL, { NULL } },
   { XMP_PREFIX_DUBLIN_CORE,  "subject",        0, NULL, { NULL } },
   { NULL,                    NULL,             0, NULL, { NULL } }
};
TestDataEntry * const import_exportdata = propertiestotest;

/**
 * This testdata tests different types in the #XMPModel and what they
 * return for the editor.
 * Note: The pos attribute is ignored.
 */
static TestDataEntry _xmp_property_values_view[] =
{
   { XMP_PREFIX_DUBLIN_CORE,  "title",      0, "Hello, World,",
     {"x-default", "Hello, World,", "ja", "\xe3\x81\x93"} },
   { XMP_PREFIX_DUBLIN_CORE,  "creator",    0, "1) Wilber, 2) Wilma",
     {"1) Wilber, 2) Wilma"} },
   { NULL,  NULL,          0, NULL, { NULL } }
};
TestDataEntry * const xmp_property_values_view = _xmp_property_values_view;


static void gimp_test_xmp_model_setup       (GimpTestFixture *fixture,
                                             gconstpointer    data);
static void gimp_test_xmp_model_teardown    (GimpTestFixture *fixture,
                                             gconstpointer    data);


/**
 * gimp_test_xmp_model_setup:
 * @fixture: GimpTestFixture fixture
 * @data:
 *
 * Test fixture to setup an XMPModel.
 **/
static void
gimp_test_xmp_model_setup (GimpTestFixture *fixture,
                           gconstpointer    data)
{
  fixture->xmp_model = xmp_model_new ();
}


static void
gimp_test_xmp_model_teardown (GimpTestFixture *fixture,
                              gconstpointer    data)
{
  g_object_unref (fixture->xmp_model);
}


/**
 * test_xmp_model_value_types
 * @fixture:
 * @data:
 *
 * Test if different value types are correctly set. The string
 * representation of the value should be correct.
 **/
static void
test_xmp_model_value_types (GimpTestFixture *fixture,
                            gconstpointer    data)
{
  int             i, j;
  guint           a, b;
  const gchar    *value;
  const gchar   **raw         = NULL;
  TestDataEntry  *testdata;
  GError        **error       = NULL;
  gchar          *uri         = NULL;

  uri = g_build_filename (g_getenv ("GIMP_TESTING_ABS_TOP_SRCDIR"),
                          "plug-ins/metadata/tests/files/test_xmp.jpg",
                          NULL);

  xmp_model_parse_file (fixture->xmp_model, uri, error);
  g_free (uri);

  for (i = 0; xmp_property_values_view[i].name != NULL; ++i)
   {
    testdata = &(xmp_property_values_view[i]);

    /* view representation for the editor */
    value = xmp_model_get_scalar_property (fixture->xmp_model,
                                           testdata->schema_name,
                                           testdata->name);
    g_assert_cmpstr (value, ==, testdata->expected_value);

    /* internal data, which the view representation is created from */
    raw = xmp_model_get_raw_property_value (fixture->xmp_model,
                                            testdata->schema_name,
                                            testdata->name);
    a = g_strv_length ((gchar **) raw);
    b = g_strv_length ((gchar **) testdata->expected_values);
    g_assert_cmpuint (a, ==, b);
    for (j = 0; raw[j] != NULL; j++)
     {
      g_assert_cmpstr (raw[j], ==, testdata->expected_values[j]);
     }
   }

}

/**
 * test_xmp_model_import_export_structures:
 * @fixture:
 * @data:
 *
 * Test to assure the round trip of data import, editing, export is
 * working.
 **/
static void
test_xmp_model_import_export_structures (GimpTestFixture *fixture,
                                         gconstpointer    data)
{
  int             i;
  gboolean        result;
  const gchar    *before_value;
  const gchar    *after_value;
  GString        *buffer;
  TestDataEntry  *testdata;
  const gchar    *scalarvalue = "test";
  GError        **error       = NULL;
  gchar          *uri         = NULL;


  uri = g_build_filename (g_getenv ("GIMP_TESTING_ABS_TOP_SRCDIR"),
                          "plug-ins/metadata/tests/files/test.xmp",
                          NULL);

  xmp_model_parse_file (fixture->xmp_model, uri, error);
  g_free (uri);

  for (i = 0; import_exportdata[i].name != NULL; ++i)
   {
    testdata = &(import_exportdata[i]);

    /* backup the original raw value */
    before_value = xmp_model_get_scalar_property (fixture->xmp_model,
                                                  testdata->schema_name,
                                                  testdata->name);
    g_assert (before_value != NULL);

    /* set a new scalar value */
    result = xmp_model_set_scalar_property (fixture->xmp_model,
                                            testdata->schema_name,
                                            testdata->name,
                                            scalarvalue);
    g_assert (result == TRUE);

    /* export */
    buffer = g_string_new ("GIMP_TEST");
    xmp_generate_packet (fixture->xmp_model, buffer);

    /* import */
    xmp_model_parse_buffer (fixture->xmp_model,
                            buffer->str,
                            buffer->len,
                            TRUE,
                            error);
    after_value = xmp_model_get_scalar_property (fixture->xmp_model,
                                                 testdata->schema_name,
                                                 testdata->name);
    /* check that the scalar value is correctly exported */
    g_assert (after_value != NULL);
    g_assert_cmpstr (after_value, ==, scalarvalue);

   }
}

/**
 * test_xmp_model_import_export:
 * @fixture:
 * @data:
 *
 * Functional test, which assures that changes in the string
 * representation is correctly merged on export. This test starts of
 * with inserting scalar values only.
 **/
static void
test_xmp_model_import_export (GimpTestFixture *fixture,
                              gconstpointer    data)
{
  int             i;
  gboolean        result;
  GString        *buffer;
  TestDataEntry  *testdata;
  const gchar   **after_value;
  const gchar    *scalarvalue = "test";
  GError        **error       = NULL;


  for (i = 0; import_exportdata[i].name != NULL; ++i)
   {
    testdata = &(import_exportdata[i]);

    /* set a new scalar value */
    result = xmp_model_set_scalar_property (fixture->xmp_model,
                                            testdata->schema_name,
                                            testdata->name,
                                            scalarvalue);
    g_assert (result == TRUE);

    /* export */
    buffer = g_string_new ("GIMP_TEST");
    xmp_generate_packet (fixture->xmp_model, buffer);

    /* import */
    xmp_model_parse_buffer (fixture->xmp_model,
                            buffer->str,
                            buffer->len,
                            TRUE,
                            error);
    after_value = xmp_model_get_raw_property_value (fixture->xmp_model,
                                                    testdata->schema_name,
                                                    testdata->name);

    /* check that the scalar value is correctly exported */
    g_assert (after_value != NULL);
    g_assert_cmpstr (after_value[testdata->pos], ==, scalarvalue);
   }
}

int main(int argc, char **argv)
{
  gint result = -1;

  g_type_init();
  g_test_init (&argc, &argv, NULL);

  ADD_TEST (test_xmp_model_import_export);
  ADD_TEST (test_xmp_model_import_export_structures);
  ADD_TEST (test_xmp_model_value_types);

  result = g_test_run ();

  return result;
}