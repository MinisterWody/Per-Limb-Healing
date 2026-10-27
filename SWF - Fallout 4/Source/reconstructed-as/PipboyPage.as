// Reconstructed from the supplied SWF. Descriptive parameter/local names are
// inferred for readability; class/member names and behavior come from bytecode.
package
{
   import Pipboy.COMPANIONAPP.MobileBackButtonEvent;
   import Shared.AS3.BSButtonHintData;
   import Shared.BGSExternalInterface;
   
   public class PipboyPage extends PipboySubMenu
   {
      
      public static const LOWER_PIPBOY_ALLOW_CHANGE:String = "PipboyPage::LowerPipboyAllowedChange";
      
      public static const BOTTOM_BAR_UPDATE:String = "PipboyPage::BottomBarUpdate";
      
      protected var _TabNames:Array;
      
      protected var _buttonHintDataV:Vector.<BSButtonHintData>;
      
      public function PipboyPage()
      {
         super();
         this._buttonHintDataV = new Vector.<BSButtonHintData>();
         this.PopulateButtonHintData();
      }
      
      public function get TabNames() : Array
      {
         return this._TabNames;
      }
      
      override public function onAddedToStage() : void
      {
         super.onAddedToStage();
         MobileBackButtonEvent.Register(stage,this.onMobileBackButtonPressed);
      }
      
      override public function onRemovedFromStage() : void
      {
         MobileBackButtonEvent.Unregister(stage,this.onMobileBackButtonPressed);
         super.onRemovedFromStage();
      }
      
      public function get buttonHintDataV() : Vector.<BSButtonHintData>
      {
         return this._buttonHintDataV;
      }
      
      protected function PopulateButtonHintData() : *
      {
      }
      
      public function CanSwitchFromCurrentPage() : Boolean
      {
         return true;
      }
      
      public function CanSwitchTabs(tabIndex:uint) : Boolean
      {
         return this._TabNames != null && tabIndex < this._TabNames.length;
      }
      
      public function CanLowerPipboy() : Boolean
      {
         return true;
      }
      
      private function onMobileBackButtonPressed(event:MobileBackButtonEvent) : void
      {
         var handled:Boolean = this.HandleMobileBackButton();
         BGSExternalInterface.call(BGSCodeObj,"onBackButtonHandled",handled);
      }
      
      protected function HandleMobileBackButton() : Boolean
      {
         return false;
      }
   }
}

