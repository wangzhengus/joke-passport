#include <assert.h>
#include <string.h>

#include "app_passport_id.h"

int main(void)
{
    char id[APP_PASSPORT_ID_MAX];

    assert(app_passport_id_format(id, sizeof(id), 2026, 9, 10, 11, 29, 0xA1B2) == 17);
    assert(strcmp(id, "J202609101129A1B2") == 0);

    assert(app_passport_id_format(id, sizeof(id), 2026, 1, 2, 3, 4, 0x000F) == 17);
    assert(strcmp(id, "J202601020304000F") == 0);

    assert(app_passport_id_format(id, 10, 2026, 9, 10, 11, 29, 0xA1B2) == -1);
    assert(app_passport_id_format(id, sizeof(id), 1999, 9, 10, 11, 29, 0xA1B2) == -1);
    assert(app_passport_id_format(NULL, sizeof(id), 2026, 9, 10, 11, 29, 0xA1B2) == -1);
    assert(app_passport_id_format(id, sizeof(id), 2026, 13, 10, 11, 29, 0xA1B2) == -1);
    return 0;
}
