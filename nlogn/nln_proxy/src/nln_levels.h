
#ifndef NLN_LEVELS_H
#define NLN_LEVELS_H

#include <string>


struct levels_entry {
    bool exists;
    int port;
};
const std::string           levels_host = "10.10.153.115";
const struct levels_entry   levels_map = {.exists = true, .port = 8090};const int levels_len  = 24;
const struct levels_entry   levels[24] = {
    {.exists = false, .port = -1  },
    {.exists = false, .port = -1  },
    {.exists = true, .port = 9092  },
    {.exists = false, .port = -1  },
    {.exists = false, .port = -1  },
    {.exists = false, .port = -1  },
    {.exists = false, .port = -1  },
    {.exists = false, .port = -1  },
    {.exists = false, .port = -1  },
    {.exists = false, .port = -1  },
    {.exists = false, .port = -1  },
    {.exists = false, .port = -1  },
    {.exists = false, .port = -1  },
    {.exists = false, .port = -1  },
    {.exists = false, .port = -1  },
    {.exists = false, .port = -1  },
    {.exists = false, .port = -1  },
    {.exists = false, .port = -1  },
    {.exists = false, .port = -1  },
    {.exists = false, .port = -1  },
    {.exists = false, .port = -1  },
    {.exists = false, .port = -1  },
    {.exists = false, .port = -1  },
    {.exists = false, .port = -1  },
};
#endif