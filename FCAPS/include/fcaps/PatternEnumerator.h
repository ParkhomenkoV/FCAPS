// Initial software, Aleksey Buzmakov, Copyright (c) Higher School of Economics, Russia, GPL v2 license, 2011-2015, v0.7

// Author: Aleksey Buzmakov
// Description: A simple interface for enumerating patterns. Every pattern is described by its image.

#ifndef PATTERNENUMERATOR_H_INCLUDED
#define PATTERNENUMERATOR_H_INCLUDED

#include <common.h>
#include <fcaps/BasicTypes.h>

////////////////////////////////////////////////////////////////////////

const char PatternEnumeratorModuleType[] = "PatternEnumeratorModules";

// Enumeration of possbile usage of the current pattern
enum TCurrentPatternUsage {
    // The super-pattern could be useful
    CPU_Expand = 0,
    // Any super-pattern is useless
    CPU_Reject,

    CPU_EnumCount
};

// A structure for encoding the image of a pattern
struct CPatternImage {
    typedef unsigned int TPatternId;

	// Unique id of the pattern
	TPatternId PatternId;
	// The cardinality of the image, known as support.
    int ImageSize;
	// The image (the set of objects) of the pattern.
    int* Objects;

	CPatternImage() :
		PatternId( -1 ),
		ImageSize( 0 ),
		Objects( 0 ) {}
};

interface IPatternEnumerator : public virtual IObject {
	// Add context to the projection chain iteratively.
	//  objectNum -- number of the object.
	virtual void AddObject( DWORD objectNum, const JSON& intent ) = 0;
    // // Returns an empty pattern, i.e., a pattern that is covered by any other pattern
    // virtual void GetEmptyPattern( CPatternImage& pattern ) = 0;
    // Returns the next pattern.
    virtual bool GetNextPattern( TCurrentPatternUsage usage, CPatternImage& pattern ) = 0;
    // Clear memory in the pattern
    virtual void ClearMemory( CPatternImage& pattern ) = 0;
};

#endif // PATTERNENUMERATOR_H_INCLUDED
