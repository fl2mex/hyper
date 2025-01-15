#include "Application.h"

int main()
{
	hyper::Spec spec{}; // Default options stored in Include.h
	spec.InfoDebug = false; // Less Verbose

	hyper::Logger* logger = new hyper::Logger();
	logger->SetDebug(spec);

	hyper::Application app(spec);
	app.Run();
}