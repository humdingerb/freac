 /* fre:ac - free audio converter
  * Copyright (C) 2001-2018 Robert Kausch <robert.kausch@freac.org>
  *
  * This program is free software; you can redistribute it and/or
  * modify it under the terms of the GNU General Public License as
  * published by the Free Software Foundation, either version 2 of
  * the License, or (at your option) any later version.
  *
  * THIS PACKAGE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR
  * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
  * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE. */

#ifndef H_FREAC_LOCKING
#define H_FREAC_LOCKING

#include <smooth.h>
#include <boca.h>

using namespace smooth;

namespace freac
{
	class Locking
	{
		private:
			static Int		 allocatedThreads;

			static Array<Bool>	 deviceLocked;
			static Array<Bool>	 outputLocked;

			static Threads::Mutex	 managementMutex;
		public:
			static Bool		 AllocateThread();
			static Bool		 FreeThread();

			static Bool		 LockDeviceForTrack(const BoCA::Track &);
			static Bool		 UnlockDeviceForTrack(const BoCA::Track &);

			static Bool		 LockOutputForTrack(const BoCA::Track &);
			static Bool		 UnlockOutputForTrack(const BoCA::Track &);
	};
};

#endif
