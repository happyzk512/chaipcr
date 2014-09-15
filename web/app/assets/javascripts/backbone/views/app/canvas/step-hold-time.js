ChaiBioTech.app.Views = ChaiBioTech.app.Views || {};

ChaiBioTech.app.Views.holdTime = function(model, parent) {

  this.model = model;
  this.parent = parent;
  this.canvas = parent.canvas;

  this.render = function() {
    this.holdTime = this.model.get("step")["hold_time"];
    this.text = new fabric.Text(this.formatHoldTime(), {
      fill: 'black',
      fontSize: 30,
      top : this.parent.top + 30,
      left: this.parent.left + 40,
      fontFamily: "Ostrich Sans",
      selectable: false
    })
    //ßåœ∑´®†¥¨ˆøπ“‘≠–ºª•¶§∞¢£™™¡`ºººººº’”.add(this.text);
  }
  this.formatHoldTime = function() {
    var holdTimeHour = Math.floor(this.holdTime / 60);
    var holdTimeMinute = (this.holdTime % 60);
    //console.log(holdTime, holdTimeHour, holdTimeMinute)
    return holdTimeHour+":"+holdTimeMinute;
  }
  this.render();
  return this;
}