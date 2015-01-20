#pragma once

#include <boost/shared_ptr.hpp>

#define xForEachActive(iter, container) \
	for(typeof(container.begin()) iter = container.begin(); iter != container.end(); iter++)	\
		if ((*iter)->parent == this)

template <class ParentType, class ChildType>
class Containee {
public:
	ParentType *parent;		/* set to null to request removal */

	Containee(): parent(NULL) {}
	virtual ~Containee() {}

protected:
	void detach() {
		parent = NULL;
	}
};

template <class ParentType, class ChildType>
class BaseContainer {

	typedef boost::shared_ptr<ChildType> ChildPtr;

public:
	void clear() {
		xForEachActive(iter, _objects) {
			(*iter)->parent = NULL;
		}
		removeFinished();
	}

	static void addChild(ParentType *parent, const ChildPtr &child) {
		parent->_objects.push_back(child);
		child->parent = parent;
		parent->removeFinished();
	}

	virtual ~BaseContainer() {
		clear();
	}

protected:
	std::vector<ChildPtr> _objects;

	void removeFinished() {
		_objects.erase(remove_if(_objects.begin(), _objects.end(), isFinished), _objects.end());
	}

private:
	static bool isFinished(const ChildPtr &object) {
		return !(object && object->parent);
	}
};

template <class ParentPtr, class ChildPtr>
class ProcessContainer: public BaseContainer<ParentPtr, ChildPtr> {
public:
	virtual ~ProcessContainer() {};

	virtual void process() {
		xForEachActive(iter, this->_objects) {
			(*iter)->process();
		}
		this->removeFinished();
	}
};

