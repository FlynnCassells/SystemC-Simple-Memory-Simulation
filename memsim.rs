struct Memory {
    data: Vec<u8>,
    latency_ns: u64,
}
#[derive(Debug)]
enum MemError {
    OutOfBounds { addr: usize, len: usize},
}
impl Memory {
    fn new(size: usize, latency_ns: u64) -> Memory{
        Memory{data: vec![0;size], latency_ns}
    }
    fn write(&mut self, addr: usize, data: &[u8])-> Result<(), MemError>{
        let n = data.len();
        self.check(addr, n)?;
        self.data[addr..addr + n].copy_from_slice(data);
        Ok(())
    }
    fn read(&self, addr: usize, buf: &mut [u8])-> Result<(), MemError>{
        let n = buf.len();
        self.check(addr, n)?;
        buf.copy_from_slice(&self.data[addr..addr + n]);
        Ok(())
    }
    fn check(&self, addr: usize, len: usize) -> Result<(), MemError>{
        if addr+len > self.data.len() {
            return Err(MemError::OutOfBounds { addr, len });
        }
        Ok(())
    }
}

fn main() {
    let mut mem = Memory::new(4096, 50);
    println!("Memory: {} bytes, latency {} ns", mem.data.len(), mem.latency_ns);
    let data = [0xDE, 0xAD, 0xBE, 0xEF];
    let mut buf = [0u8;4];
    
    match mem.write(0x10, &data) {
        Ok(()) => println!("worked"),
        Err(e) => println!("failed: {:?}", e),
    }
    match mem.read(0x10, &mut buf) {
        Ok(()) => println!("worked"),
        Err(e) => println!("failed: {:?}", e),
    }
    println!("Currently stored in memory: {:02X?}", buf);
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn new_memory_is_zeroed(){
        let mem = Memory::new(0x10,50);
        let mut buf = [1u8;0x10];
        const EXPECTED: [u8; 0x10]=[0u8;0x10];
        mem.read(0, &mut buf).unwrap();
        assert_eq!(buf, EXPECTED);
    }
    #[test]
    fn write_then_read_back(){
        const EXPECTED: [u8; 4] = [0xDE, 0xAD, 0xBE, 0xEF];
        let mut mem = Memory::new(4096, 50);
        let mut buf = [0u8;4];
        mem.write(0x10, &EXPECTED).unwrap();
        mem.read(0x10, &mut buf).unwrap();
        assert_eq!(buf, EXPECTED);
    }
    #[test]
    fn last_valid_address_works(){
        let mem = Memory::new(4096, 50);
        let mut buf = [1u8; 4];
        assert!(mem.read(4092, &mut buf).is_ok());
    }
    #[test]
    fn one_past_end_fails(){
        let mem = Memory::new(4096, 50);
        let mut buf = [0u8; 4];
        assert!(matches!(mem.read(4093, &mut buf), Err(MemError::OutOfBounds{..})));
    }
}