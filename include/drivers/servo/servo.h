#pragma once

struct servo {
 int (*set_angle)(struct servo *, int);
 int (*destroy)(struct servo *);
};
