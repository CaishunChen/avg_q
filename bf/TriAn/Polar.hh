// Copyright (C) 1996 Bernd Feige
// 
// This file is part of avg_q.
// 
// avg_q is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// avg_q is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with avg_q.  If not, see <http://www.gnu.org/licenses/>.
#ifndef _Polar_HH
#define _Polar_HH

#include "General.hh"
#include "Point.hh"

class Polar {
public:
 Polar();
 Polar(Point* p);
 virtual ~Polar() {};
 Point to_Point();
 void print();
//private:
 float r;
 float theta;
 float phi;
};
#endif
