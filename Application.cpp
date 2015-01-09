#include "Application.h"

namespace Application {
	trackerStatus status = STATUS_IDLE;
	bool isTrackerCalibrated = false;
	CvRect *faceRectangle = NULL;

	std::vector<boost::shared_ptr<AbstractStore> > getStores() {
		static std::vector<boost::shared_ptr<AbstractStore> > stores;

		if (stores.size() < 1) {
			stores.push_back(boost::shared_ptr<AbstractStore>(new SocketStore()));
			stores.push_back(boost::shared_ptr<AbstractStore>(new StreamStore(std::cout)));
			stores.push_back(boost::shared_ptr<AbstractStore>(new WindowStore(WindowPointer::PointerSpec(20, 30, 0, 0, 1), WindowPointer::PointerSpec(20, 20, 0, 1, 1), WindowPointer::PointerSpec(30, 30, 1, 0, 1))));
		}

		return stores;
	}
}

