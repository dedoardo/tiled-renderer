/* frame.hpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
#pragma once

// camy
#include <camy/core/base.hpp>
#include <camy/core/memory/vector.hpp>
#include <camy/graphics/ibucket.hpp>
#include <camy/graphics/base.hpp>

namespace camy
{
	/*
		Groups and flushes compiled buckets
	*/
	class camy_api Frame final
	{
	public:
		Frame();
		~Frame();

		void add_bucket(IBucket& bucket, rsize pass);
		void remove_bucket(IBucket& bucket);
		void render();

	private:
		Vector<IBucket*> m_buckets[kMaxPasses];
	};
}