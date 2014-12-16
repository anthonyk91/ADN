#ifndef DISPLAYING_H_
#define DISPLAYING_H_

#include <sstream>

namespace utils
{

/** interface class for elements that give output, inc some shortcuts **/


class Displaying
{
public:
	virtual ~Displaying() {}
	
	virtual std::string displayStr() const;
	virtual void display(std::ostream&) const = 0;
};

// overload operator << to allow displaying in-line
std::ostream& operator<< (std::ostream& out, const Displaying& val );

} // namespace utils

#endif /*DISPLAYING_H_*/
