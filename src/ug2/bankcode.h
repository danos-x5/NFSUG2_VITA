#ifndef UG2_BANKCODE_H_
#define UG2_BANKCODE_H_

// Runs the x86 routines inside loaded EA audio banks (bankcode.cpp).

namespace ug2 { namespace bankcode {

// Lets calls through pointers reach loaded bank routines; once.
void install();

}}

#endif /* !UG2_BANKCODE_H_ */
