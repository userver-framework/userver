/**
        curl-ev: wrapper for integrating libcurl with libev applications
        Copyright (c) 2013 Oliver Kuckertz <oliver.kuckertz@mologie.de>
        See COPYING for license information.
*/

#pragma once

#if !defined CURLASIO_API
#if defined _MSC_VER
#if defined CURLASIO_STATIC
#define CURLASIO_API
#elif defined libcurlasio_EXPORTS
#define CURLASIO_API __declspec(dllexport)
#else
#define CURLASIO_API __declspec(dllimport)
#endif
#else
#define CURLASIO_API
#endif
#endif

// Disable unused boost components. If your program depends on any of these
// components, include them prior to including curl-ev.
#ifndef BOOST_ASIO_DISABLE_BOOST_REGEX
#define BOOST_ASIO_DISABLE_BOOST_REGEX
#endif
