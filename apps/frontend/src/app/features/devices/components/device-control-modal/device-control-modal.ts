import {CommonModule} from '@angular/common';
import {Component, EventEmitter, Input, OnChanges, Output} from '@angular/core';
import {FormsModule} from '@angular/forms';
import {DeviceControlChange, DeviceControlVm} from '../../utils/device-detail-view-model.util';


@Component({
  selector: 'app-device-control-modal',
  standalone: true,
  imports: [CommonModule, FormsModule],
  templateUrl: './device-control-modal.html',
  styleUrl: './device-control-modal.scss'
})
export class DeviceControlModal implements OnChanges {

  @Input({required: true})
  control!: DeviceControlVm;

  @Output()
  cancel = new EventEmitter<void>();

  @Output()
  apply = new EventEmitter<DeviceControlChange>();

  switchDraftValue = 'OFF';
  numberDraftValue: string | number | null = null;

  ngOnChanges(): void {
    if (!this.control) {
      return;
    }

    if (this.control.kind === 'switch') {
      this.switchDraftValue = String(this.control.currentValue ?? 'OFF');
      return;
    }

    if (this.control.kind === 'number') {
      this.numberDraftValue = this.control.currentValue === undefined
      || this.control.currentValue === null
      || typeof this.control.currentValue === 'boolean'
        ? this.control.min ?? 0
        : this.control.currentValue;
    }
  }

  applyChange(): void {
    this.apply.emit({
      control: this.control,
      value: this.normalizeValue()
    });
  }

  private normalizeValue(): string | number | boolean | null {
    if (this.control.kind === 'switch') {
      return this.switchDraftValue;
    }

    if (this.control.kind === 'number') {
      const numericValue = Number(this.numberDraftValue);

      if (!Number.isFinite(numericValue)) {
        return this.control.min ?? 0;
      }

      return numericValue;
    }

    return null;
  }
}
