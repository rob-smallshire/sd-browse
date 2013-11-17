/*
 * sdcard.hpp
 *
 *  Created on: 14 Nov 2013
 *      Author: rjs
 */

#ifndef SDCARD_HPP_
#define SDCARD_HPP_

#include <SdFat.h>

namespace sd {

    void initialize();
    SdFile& root();
}

#endif /* SDCARD_HPP_ */
