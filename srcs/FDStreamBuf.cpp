/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   FDStreamBuf.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jruhlman <jruhlman@student.42mulhouse.fr>  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/05 20:26:48 by jruhlman          #+#    #+#             */
/*   Updated: 2025/02/09 09:53:14 by jruhlman         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "FDStreamBuf.hpp"

FDStreamBuf::FDStreamBuf(int fd, std::size_t bufferSize) : _fd(fd), _bufferSize(bufferSize)
{
	_buffer = new char[_bufferSize];
	setg(_buffer, _buffer, _buffer);
}
FDStreamBuf::~FDStreamBuf(void) 
{
	delete[] _buffer;
}
std::string FDStreamBuf::readAll(void)
{
	std::string result;
	while (true){
		int c = this->sgetc();
		if (c == traits_type::eof())
			break;
		result.push_back(static_cast<char>(c));
		this->sbumpc();
	}
	return (result);
}

int FDStreamBuf::underflow()
{
    if (gptr() < egptr())
        return traits_type::to_int_type(*gptr());
    ssize_t n = read(_fd, _buffer, _bufferSize);
    if (n <= 0)
        return traits_type::eof();
    setg(_buffer, _buffer, _buffer + n);
    return traits_type::to_int_type(*gptr());
}
