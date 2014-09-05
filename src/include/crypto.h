#pragma once

#include "global.h"
#include "ioutil.h"
#include <crypto_box.h>

typedef std::pair<std::string, std::string> stringpair_t;

static stringpair_t generate_keypair() {
    std::string pub, priv;

    pub = crypto_box_keypair(&priv);

    return stringpair_t(priv, pub);
}
