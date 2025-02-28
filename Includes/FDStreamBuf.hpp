/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   FDStreamBuf.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jruhlman <jruhlman@student.42mulhouse.fr>  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/05 20:15:49 by jruhlman          #+#    #+#             */
/*   Updated: 2025/02/05 21:36:11 by jruhlman         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Webserver.hpp"

class FDStreamBuf : public std::streambuf
{
	private:
		int	_fd;
		char *_buffer;
		std::size_t _bufferSize;

		FDStreamBuf(const FDStreamBuf& other);
		FDStreamBuf& operator=(const FDStreamBuf& other);
	public:
		FDStreamBuf(int fd, std::size_t bufferSize);
		virtual ~FDStreamBuf(void);
		std::string readAll(void);
	protected:
		virtual int underflow(void);
};
