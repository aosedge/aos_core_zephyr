/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "xrun.h"

int xrun_run(const char* bundle, int console_socket, const char* container_id)
{
    return 0;
}

int xrun_pause(const char* container_id)
{
    return 0;
}

int xrun_resume(const char* container_id)
{
    return 0;
}

int xrun_kill(const char* container_id)
{
    return 0;
}

int xrun_state(const char* container_id, enum container_status* state)
{
    return 0;
}
