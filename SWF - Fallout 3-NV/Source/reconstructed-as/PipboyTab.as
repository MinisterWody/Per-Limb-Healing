// Reconstructed from the supplied SWF. Descriptive parameter/local names are
// inferred for readability; class/member names and behavior come from bytecode.
package
{
   import Shared.AS3.BSButtonHintData;
   
   public class PipboyTab extends PipboySubMenu
   {
      
      private var _TabIndex:uint;
      
      public function PipboyTab()
      {
         super();
      }
      
      public function get TabIndex() : uint
      {
         return this._TabIndex;
      }
      
      public function set TabIndex(tabIndex:uint) : *
      {
         this._TabIndex = tabIndex;
      }
      
      override protected function onPipboyChangeEvent(event:PipboyChangeEvent) : void
      {
         super.onPipboyChangeEvent(event);
         this.visible = event.DataObj.CurrentTab == this._TabIndex;
      }
      
      public function PopulateButtonHintData(buttonHints:Vector.<BSButtonHintData>) : *
      {
      }
   }
}

