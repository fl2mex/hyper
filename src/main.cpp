#include "Application.h"

int main()
{
	hyper::Spec spec{}; // Default options stored in Include.h
	
	hyper::Logger* logger = hyper::Logger::GetLogger();
	logger->SetDebug(spec);

	hyper::Application app(spec);
	app.Run();
}