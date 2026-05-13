#ifndef __WEBRA_API_LIST_TEST_H__
#define __WEBRA_API_LIST_TEST_H__

#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

////////////////////////////////////////////////////////////////////////////////
WEBRA_HOST_RETURN_CODE webra_api_test_get_func();
WEBRA_HOST_RETURN_CODE webra_api_test_post_func();
WEBRA_HOST_RETURN_CODE webra_api_test_session_func();
WEBRA_HOST_RETURN_CODE webra_api_test_download_func();

////////////////////////////////////////////////////////////////////////////////
WEBRA_API_PARAM webra_api_test_get_param[] = {
  {NULL,}
};

WEBRA_API_PARAM webra_api_test_post_param[] = {
  {"string", WEBRA_PARAM_STRING, WEBRA_IS_OPTIONAL, 0, 8,  WEBRA_VALID_NONE},
  {NULL,}
};

WEBRA_API_PARAM webra_api_test_session_param[] = {
  {"session1", WEBRA_PARAM_STRING, WEBRA_IS_OPTIONAL, 0, 8,  WEBRA_VALID_NONE},
  {"session2", WEBRA_PARAM_STRING, WEBRA_IS_OPTIONAL, 0, 8,  WEBRA_VALID_NONE},
  {"session3", WEBRA_PARAM_STRING, WEBRA_IS_OPTIONAL, 0, 8,  WEBRA_VALID_NONE},
  {NULL,}
};

WEBRA_API_PARAM webra_api_test_download_param[] = {
  {"filename", WEBRA_PARAM_STRING, WEBRA_IS_OPTIONAL, 0, 128,  WEBRA_VALID_NONE},
  {NULL,}
};

////////////////////////////////////////////////////////////////////////////////
WEBRA_API_PROC webra_api_test_get_proc = {
  webra_api_test_get_param,
  &webra_api_test_get_func,
  WEBRA_DONT_SAVE_DB,
  WEBRA_GROUP_ADMIN | WEBRA_GROUP_MANAGER,
  WEBRA_AUTH_NONE
};

WEBRA_API_PROC webra_api_test_post_proc = {
  webra_api_test_post_param,
  &webra_api_test_post_func,
  WEBRA_DONT_SAVE_DB,
  WEBRA_GROUP_ADMIN | WEBRA_GROUP_MANAGER,
  WEBRA_AUTH_NONE
};

WEBRA_API_PROC webra_api_test_session_proc = {
  webra_api_test_session_param,
  &webra_api_test_session_func,
  WEBRA_DONT_SAVE_DB,
  WEBRA_GROUP_ADMIN | WEBRA_GROUP_MANAGER,
  WEBRA_AUTH_NONE
};

WEBRA_API_PROC webra_api_test_download_proc = {
  webra_api_test_download_param,
  &webra_api_test_download_func,
  WEBRA_DONT_SAVE_DB,
  WEBRA_GROUP_ADMIN | WEBRA_GROUP_MANAGER,
  WEBRA_AUTH_NONE
};
////////////////////////////////////////////////////////////////////////////////

#endif // __WEBRA_API_LIST_TEST_H__
