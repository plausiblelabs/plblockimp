PLBlockIMP
-----------


PLBlockIMP provides an open-source implementation of [imp_implementationWithBlock()](http://www.friday.com/bbum/2011/03/17/ios-4-3-imp_implementationwithblock/), using
vm_remap()-based trampolines as described in [Implementing imp_implementationWithBlock](http://landonf.bikemonkey.org/code/objc/imp_implementationWithBlock.20110413.html).

PLBlockIMP will generate a lightweight, high-performance function pointer trampoline that may be used to register a block as an Objective-C method implementation. Mac OS X 10.6+ and
iOS 4.0+ are supported.

Additionally, PLBlockIMP contains a generic trampoline allocator and set of generator scripts that may be used to implement custom trampoline pages on Mac OS X and iOS.

PLBlockIMP is released under the MIT license.

Sample Use
-----------

Use a block to add a new method to NSObject (based on Mike Ash's MABlockClosure example):

    int captured = 42;
    id block = ^(id self) { NSLog(@"captured is %d", captured); };
    block = [block copy];
    class_addMethod([NSObject class], @selector(my_printCaptured), pl_imp_implementationWithBlock(block), "v@:");

Additionally, pl_imp_getBlock() may be used to fetch the block associated with an IMP trampoline, and pl_imp_removeBlock() to discard a trampoline
and its associated block reference.
