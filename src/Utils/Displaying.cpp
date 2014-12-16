#include "Displaying.h"

namespace utils {

std::string Displaying::displayStr() const
 { 
	 std::ostringstream oss; 
	 display(oss); 
	 return oss.str(); 
 }

std::ostream& operator<< (std::ostream& out, const Displaying& val )
{
	val.display(out);
	return out;
}

}
