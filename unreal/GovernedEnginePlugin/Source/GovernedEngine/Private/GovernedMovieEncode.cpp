#include "GovernedMovieEncode.h"

FGovernedMovieContainerEncode& GetGovernedMovieContainerEncode()
{
	static FGovernedMovieContainerEncode Delegate;
	return Delegate;
}
