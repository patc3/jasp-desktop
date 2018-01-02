//
// Copyright (C) 2013-2018 University of Amsterdam
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "engine.h"

using namespace std;

int main(int argc, char *argv[])
{
    Engine *e = new Engine();

	if (argc > 1)
	{
		unsigned long slaveNo = 0;
		slaveNo = strtoul(argv[1], NULL, 10);
		e->setSlaveNo(slaveNo);
	}

    e->run();
}
