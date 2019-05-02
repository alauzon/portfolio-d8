CKEDITOR.plugins.add( 'fhcsssrn',
{
	init: function( editor )
	{
		editor.addCommand( 'insertFHListeEnCoeurs',
				{
					exec : function( editor )
					{    
						editor.insertHtml( '<ul class="coeurs"><li></li></ul>' );
					}
				});
	
		editor.ui.addButton( 'FHListeEnCoeurs',
				{
					label: 'Insérer une liste en coeurs',
					command: 'insertFHListeEnCoeurs',
					icon: this.path + 'images/coeur.gif'
				} );
	}
} );

