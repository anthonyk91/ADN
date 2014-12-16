#ifndef SINGLETON_H_
#define SINGLETON_H_
#include <stdexcept>

namespace utils {
	
class SingletonNoInstException : public std::runtime_error {
	public:
		SingletonNoInstException(const std::string& msg) 
            : runtime_error(std::string("Singleton instantiation error- no instance exists: ").append(msg)) {}
};

class SingletonInstExistsException : public std::runtime_error {
	public:
		SingletonInstExistsException(const std::string& msg) 
            : runtime_error(std::string("Singleton instantiation error- attempt to create instance when already exists: ").append(msg)) {}
};

} // namespace utils

#endif /*SINGLETON_H_*/
