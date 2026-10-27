// Reconstructed from the supplied SWF. Descriptive parameter/local names are
// inferred for readability; class/member names and behavior come from bytecode.
package
{
   import Shared.AS3.BSScrollingList;
   import Shared.AS3.BSUIComponent;
   import flash.geom.ColorTransform;
   
   public class PipboySubMenu extends BSUIComponent
   {
      
      private static const READ_ONLY_LIST_COLOR_MULTIPLIER:Number = 0.5;
      
      public static const READ_ONLY_NONE:int = 0;
      
      public static const READ_ONLY_DEFAULT:int = 1;
      
      public static const READ_ONLY_OFFLINE:int = 2;
      
      public static const READ_ONLY_DEMO:int = 3;
      
      protected var BGSCodeObj:Object;
      
      protected var _ReadOnlyMode:Boolean;
      
      public var _ReadOnlyModeType:int;
      
      public function PipboySubMenu()
      {
         super();
         this._ReadOnlyMode = false;
         this._ReadOnlyModeType = READ_ONLY_NONE;
      }
      
      protected static function SetScrollingListReadOnly(list:BSScrollingList, readOnly:Boolean) : void
      {
         var listColorTransform:ColorTransform = list.transform.colorTransform;
         var colorMultiplier:Number = readOnly ? READ_ONLY_LIST_COLOR_MULTIPLIER : 1;
         listColorTransform.redMultiplier = colorMultiplier;
         listColorTransform.greenMultiplier = colorMultiplier;
         listColorTransform.blueMultiplier = colorMultiplier;
         list.transform.colorTransform = listColorTransform;
      }
      
      public function InitCodeObj(nativeCodeObject:Object) : *
      {
         this.BGSCodeObj = nativeCodeObject;
      }
      
      public function ReleaseCodeObj() : *
      {
         this.BGSCodeObj = null;
      }
      
      public function get codeObj() : Object
      {
         return this.BGSCodeObj;
      }
      
      override public function onAddedToStage() : void
      {
         super.onAddedToStage();
         PipboyChangeEvent.Register(stage,this.onPipboyChangeEventInternal);
      }
      
      override public function onRemovedFromStage() : void
      {
         PipboyChangeEvent.Unregister(stage,this.onPipboyChangeEventInternal);
         this.ReleaseCodeObj();
         super.onRemovedFromStage();
      }
      
      private function onPipboyChangeEventInternal(event:PipboyChangeEvent) : void
      {
         if(event.UpdateMask.Intersects(PipboyUpdateMask.ReadOnly))
         {
            if(this._ReadOnlyModeType != event.DataObj.ReadOnlyMode)
            {
               this._ReadOnlyMode = event.DataObj.ReadOnlyMode != READ_ONLY_NONE;
               this._ReadOnlyModeType = event.DataObj.ReadOnlyMode;
               this.onReadOnlyChanged(this._ReadOnlyMode);
            }
         }
         if(event.UpdateMask.Intersects(this.GetUpdateMask()))
         {
            this.onPipboyChangeEvent(event);
         }
      }
      
      protected function onPipboyChangeEvent(event:PipboyChangeEvent) : void
      {
      }
      
      protected function onReadOnlyChanged(readOnly:Boolean) : void
      {
      }
      
      public function ProcessUserEvent(eventName:String, isPressed:Boolean) : Boolean
      {
         return false;
      }
      
      public function GetIsUsingRightStick() : Boolean
      {
         return false;
      }
      
      public function onRightThumbstickInput(direction:uint) : *
      {
      }
      
      protected function GetUpdateMask() : PipboyUpdateMask
      {
         return PipboyUpdateMask.None;
      }
   }
}

