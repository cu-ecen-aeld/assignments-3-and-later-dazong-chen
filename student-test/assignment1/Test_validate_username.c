#include "unity.h"
#include <stdbool.h>
#include <stdlib.h>
#include "../../examples/autotest-validate/autotest-validate.h"
#include "../../assignment-autotest/test/assignment1/username-from-conf-file.h"

/**
* This function should:
*   1) Call the my_username() function in Test_assignment_validate.c to get your hard coded username.
*   2) Obtain the value returned from function malloc_username_from_conf_file() in username-from-conf-file.h within
*       the assignment autotest submodule at assignment-autotest/test/assignment1/
*   3) Use unity assertion TEST_ASSERT_EQUAL_STRING_MESSAGE the two strings are equal.  See
*       the [unity assertion reference](https://github.com/ThrowTheSwitch/Unity/blob/master/docs/UnityAssertionsReference.md)
*/
void test_validate_my_username()
{
    const char*	auto_test_user_name;
    char* 		conf_file_user_name;
    char 		message[30] = "---> NOT MATCH! <---";
    
    
    
    auto_test_user_name = my_username();
    conf_file_user_name = malloc_username_from_conf_file();
    
    TEST_ASSERT_EQUAL_STRING_MESSAGE(auto_test_user_name, conf_file_user_name, message);
}
