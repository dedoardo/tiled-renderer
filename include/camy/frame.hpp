/* frame.hpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
#pragma once

// camy
#include <camy/graphics.hpp>
#include <camy/config.hpp>
#include <camy/containers/vector.hpp>

namespace camy
{
	class IBucket;

	/*
		Groups and flushes compiled buckets
	*/
	class CAMY_API Frame final
	{
	public:
		Frame();
		~Frame();

		void add_bucket(IBucket& bucket, rsize pass);
		void remove_bucket(IBucket& bucket);
		void render();

	private:
		Vector<IBucket*> m_buckets[API::MAX_PASSES];
	};
}