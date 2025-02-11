//! @file renderer.cpp
//! @brief Implementation of Substance renderer class
//! @author Christophe Soum - Allegorithmic
//! @copyright Allegorithmic. All rights reserved.

#include "SubstanceCorePrivatePCH.h"

#include "SubstanceFGraph.h"

#include "framework/details/detailsrendererimpl.h"
#include "framework/details/detailslinkdata.h"
#include "framework/renderer.h"


DEFINE_LOG_CATEGORY_STATIC(LogSbsRender, Warning, All);


Substance::Renderer::Renderer(const RenderOptions& renderOptions) :
	mRendererImpl(new Details::RendererImpl(renderOptions))
{
}


Substance::Renderer::~Renderer()
{
	delete mRendererImpl;
}


void Substance::Renderer::push(Substance::FGraphInstance* graph)
{
	Details::LinkDataAssembly *linkdata =
		(Details::LinkDataAssembly *)graph->Desc->Parent->getLinkData().get();

	if (0 == linkdata || 0 == linkdata->getSize())
	{
		UE_LOG(LogSbsRender, Error, TEXT("Cannot render Graph Instance %s, no link data, check Instance Factory (%s) cook options."),
			*graph->ParentInstance->GetName(), *graph->Desc->Parent->Parent->GetName());
		return;
	}
	
	mRendererImpl->push(graph);
}


void Substance::Renderer::push(Substance::List<Substance::FGraphInstance*>& graphs)
{
	Substance::List<Substance::FGraphInstance*>::TIterator
		ItGraph(graphs.itfront());

	for (;ItGraph;++ItGraph)
	{
		push(*ItGraph);
	}
}


int32 Substance::Renderer::run(uint32 runOptions)
{
	return mRendererImpl->run(runOptions);
}


bool Substance::Renderer::cancel(uint32 runUid)
{
	return mRendererImpl->cancel(runUid);
}


void Substance::Renderer::cancelAll()
{
	mRendererImpl->cancel();
}


void Substance::Renderer::flush()
{
	mRendererImpl->flush();
}


void Substance::Renderer::clearCache()
{
	mRendererImpl->clearCache();
}


bool Substance::Renderer::isPending(uint32 runUid) const
{
	return mRendererImpl->isPending(runUid);
}


void Substance::Renderer::hold()
{
	mRendererImpl->hold();
}


void Substance::Renderer::resume()
{
	mRendererImpl->resume();
}


void Substance::Renderer::setRenderCallbacks(RenderCallbacks* callbacks)
{
	mRendererImpl->setRenderCallbacks(callbacks);
}
