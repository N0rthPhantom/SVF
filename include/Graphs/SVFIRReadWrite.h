//===- SVFIRReadWrite.h -- SVFIR read & write-----------------------------------//
//
//                     SVF: Static Value-Flow Analysis
//
// Copyright (C) <2013-2022>  <Yulei Sui>
//

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//===----------------------------------------------------------------------===//

/*
 * SVFIRReadWrite.h
 *
 *  Created on: July 30, 2022
 *      Author: Yuhao Gao
 */

#ifndef SVFIRREADWRITE_H_
#define SVFIRREADWRITE_H_

#include "MemoryModel/SVFIR.h"
#include "Util/cJSON.h"
#include <fstream>

namespace SVF
{

class SVFIRReadWrite
{
    
    public:

        /// Export SVFIR to a file
        void static exportSVFIRToFile(SVF::SVFIR* pag, std::string fileName);

};

}

#endif /* SVFIRREADWRITE_H_ */
