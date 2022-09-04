#include "events_def.h"
#include <iostream>
#include <sstream>

std::ostream& MobileCodeReqEv::dump(std::ostream& out) const {
	out << "MobileCodeReq sn = " << get_sn() << ",";
	out << "mobile=" << msg_.mobile() << std::endl;
	return out;
}

std::ostream& MobileCodeRspEv::dump(std::ostream& out) const {
	out << "MobileCodeReqEV sn = " << get_sn() << ",";
	out << "code=" << msg_.code() << std::endl;
	out << "icode=" << msg_.icode() << std::endl;
	out << "describe=" << msg_.data() << std::endl;
	return out;
}

std::ostream& LoginReqEv::dump(std::ostream& out) const {
	out << "LoginReqEv sn = " << get_sn() << ",";
	out << "mobile=" << msg_.mobile() << std::endl;
	out << "icode=" << msg_.icode() << std::endl;
	//out << "describe=" << msg_.data() << std::endl;
	return out;
}

std::ostream& LoginResEv::dump(std::ostream& out) const {
	out << "LoginResEv sn = " << get_sn() << ",";
	out << "code=" << msg_.code() << std::endl;
	//out << "icode=" << msg_.icode() << std::endl;
	out << "describe=" << msg_.desc() << std::endl;
	return out;
}