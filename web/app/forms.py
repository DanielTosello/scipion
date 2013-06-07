'''
Created on Jun 7, 2013

@author: antonio
'''
from django import forms

class HostForm(forms.Form):
    scpnHosts = forms.ChoiceField(label='Scipion hosts', widget = forms.Select(), required = False,)
    label = forms.CharField(label='Label', 
                            required=True, 
                            error_messages={'required': 'Please, enter label for the configuration'},
                            widget=forms.TextInput(attrs={'class' : 'generalInput'}))
    hostName = forms.CharField(label='Host name', 
                               required=True, 
                               error_messages={'required': 'Please, enter host name'},
                                widget=forms.TextInput(attrs={'class' : 'generalInput'}))
    userName = forms.CharField(label='User name', required=False,
                                widget=forms.TextInput(attrs={'class' : 'generalInput'}))
    hostpath = forms.CharField(label='Host path', required=True, error_messages={'required': 'Please, enter your host path'},
                                widget=forms.TextInput(attrs={'class' : 'generalInput'}))