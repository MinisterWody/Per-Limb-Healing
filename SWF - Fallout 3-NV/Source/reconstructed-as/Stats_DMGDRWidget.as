// Reconstructed from the supplied SWF. Descriptive parameter/local names are
// inferred for readability; class/member names and behavior come from bytecode.
package
{
   import flash.display.MovieClip;

   public class Stats_DMGDRWidget extends MovieClip
   {
      
      public var Icon_mc:MovieClip;
      
      private const ENTRY_SPACING:uint = 3;
      
      public function Stats_DMGDRWidget()
      {
         super();
      }
      
      public function redraw(isDamage:Boolean, values:Array) : *
      {
         var entryIndex:int = 0;
         while(this.numChildren > 1)
         {
            this.removeChildAt(this.numChildren - 1);
         }
         this.Icon_mc.gotoAndStop(isDamage ? "Weapon" : "Armor");
         var nextEntryX:Number = this.Icon_mc.x + this.Icon_mc.width + this.ENTRY_SPACING;
         if(values.length == 0)
         {
            nextEntryX = this.AddEntry(isDamage,{
               "type":1,
               "value":0
            },nextEntryX);
         }
         else
         {
            entryIndex = 0;
            while(entryIndex < values.length)
            {
               if(values[entryIndex].value > 0)
               {
                  nextEntryX = this.AddEntry(isDamage,values[entryIndex],nextEntryX);
               }
               entryIndex++;
            }
         }
      }
      
      private function AddEntry(isDamage:Boolean, entryData:Object, entryX:Number) : Number
      {
         var entry:Stats_DMGDRWidget_Entry = new Stats_DMGDRWidget_Entry();
         entry.redraw(isDamage,entryData.type,entryData.value);
         this.addChild(entry);
         entry.x = entryX;
         return entryX + entry.width + this.ENTRY_SPACING;
      }
   }
}

