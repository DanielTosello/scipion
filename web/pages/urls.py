from django.conf.urls import patterns, include, url
# Uncomment the next two lines to enable the admin:
from django.contrib import admin
admin.autodiscover()
from django.conf import settings

urlpatterns = patterns('',
    (r'^resources/(?P<path>.*)$', 'django.views.static.serve', {'document_root': settings.MEDIA_ROOT}),
    (r'^static/(?P<path>.*)$', 'django.views.static.serve', {'document_root': settings.STATIC_ROOT}),
                
    # Examples:
    # url(r'^pages/', include('pages.foo.urls')),

    # Uncomment the admin/doc line below to enable admin documentation:
    # url(r'^pages/doc/', include('django.contrib.admindocs.urls')),

    # Uncomment the next line to enable the admin:
    url(r'^admin/', include(admin.site.urls)),
    
    url(r'^projects/', 'app.views.projects'),
    url(r'^project_content/$', 'app.views.project_content'),
#    url(r'^form/$', 'app.views.form'),
    url(r'^formTable/$', 'app.views.formTable'),
    url(r'^protocol/$', 'app.views.protocol'), 
    url(r'^browse_objects/$', 'app.views.browse_objects'),
)
