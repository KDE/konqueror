
#include "konqbrowseriface.h"
#include "konqview.h"

KonqBrowserInterface::KonqBrowserInterface( KonqView *view )
    : KParts::BrowserInterface( view )
{
    m_view = view;
}

uint KonqBrowserInterface::historyLength() const
{
    return m_view->historyLength();
}

void KonqBrowserInterface::goHistory( int steps )
{
    m_view->goHistory( steps );
}

#include "konqbrowseriface.moc"

