//
//  typedefs.hpp
//  2DProject
//
//  Created by Xavier Slattery.
//  Copyright © 2015 Xavier Slattery. All rights reserved.
//

#ifndef _TYPEDEFS_HPP
#define _TYPEDEFS_HPP

#include <iostream>
#define LOG(...) 		std::cout 					<< __VA_ARGS__ ;	// These will later be change to store information.
#define WARNING(...) 	std::cout << "WARNING: "	<< __VA_ARGS__ ;	// The logs will be prefixed with the log type.
#define ERROR(...) 		std::cout << "ERROR: " 		<< __VA_ARGS__ ;	// They will also be numbered in the order they are recieved.

#endif