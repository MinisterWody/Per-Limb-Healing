// Reconstructed from the supplied SWF. Descriptive parameter/local names are
// inferred for readability; class/member names and behavior come from bytecode.
package
{

   public class Pipboy_StatsPage extends PipboyPage
   {
      
      public var StatusTab_mc:PipboyTab;
      
      public var SPECIALTab_mc:PipboyTab;
      
      public var PerksTab_mc:PipboyTab;
      
      private var tabs:Vector.<PipboyTab>;

      private var _F4SEObj:Object;
      
      public function Pipboy_StatsPage()
      {
         super();
         _TabNames = new Array("$PipboyConditionCategory","$PipboySPECIALCategory","$PipboyPerksCategory");
         this.tabs = new <PipboyTab>[this.StatusTab_mc,this.SPECIALTab_mc,this.PerksTab_mc];
         var tabIndex:uint = 0;
         while(tabIndex < this.tabs.length)
         {
            this.tabs[tabIndex].TabIndex = tabIndex;
            this.tabs[tabIndex].PopulateButtonHintData(_buttonHintDataV);
            tabIndex++;
         }
      }
      
      public function onF4SEObjCreated(codeObject:Object) : void
      {
         this._F4SEObj = codeObject;
      }

      public function GetF4SEObject() : Object
      {
         return this._F4SEObj;
      }

      override public function InitCodeObj(nativeCodeObject:Object) : *
      {
         var tab:*;
         super.InitCodeObj(nativeCodeObject);
         for each(tab in this.tabs)
         {
            tab.InitCodeObj(nativeCodeObject);
         }
      }
      
      override public function ReleaseCodeObj() : *
      {
         var tab:*;
         for each(tab in this.tabs)
         {
            tab.ReleaseCodeObj();
         }
         super.ReleaseCodeObj();
      }
      
      override public function ProcessUserEvent(eventName:String, isPressed:Boolean) : Boolean
      {
         var tab:*;
         var handled:Boolean = false;
         for each(tab in this.tabs)
         {
            if(tab.visible)
            {
               handled = (tab.ProcessUserEvent(eventName,isPressed));
            }
         }
         return handled;
      }
   }
}

